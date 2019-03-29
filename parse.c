#include "parse.h"
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

long long int my_strtoll(const char *nptr, const char **endptr, int base);
double my_strtod(const char *string, const char **endPtr);

static struct expr *parse_expr(struct parse_ctx *ctx);
static struct expr *parse_atom(struct parse_ctx *ctx);


#define PARSE_ERR(...) \
    do { \
        error_emit(ctx->err_ctx, ERROR_CATEGORY_ERROR, ctx->token_text, __VA_ARGS__); \
        longjmp(ctx->error_jmp_buf, 1); \
    } while(0)

#define NEXT_TOKEN() \
    do { \
        ctx->prev_token_text = ctx->token_text; \
        ctx->token = scan_next_token(&ctx->scan_ctx, &ctx->token_text.ptr); \
        ctx->token_text.len = (int)(ctx->scan_ctx.cursor - ctx->token_text.ptr); \
    } while (0)


static struct expr *expr_create(struct parse_ctx *ctx, uint expr_type, slice_t source_text) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->kind = expr_type;
    e->source_text = source_text;
    return e;
}
static struct expr *bool_create(struct parse_ctx *ctx, bool bool_value, slice_t source_text) {
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    e->c.tag = &type_bool;
    e->c.boolean = bool_value;
    return e;
}
static struct expr *unit_create(struct parse_ctx *ctx, slice_t source_text) {
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    e->c.tag = &type_unit;
    return e;
}
static struct expr *prim_create_bin(struct parse_ctx *ctx, int prim, struct expr *arg0, struct expr *arg1) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, slice_span(arg0->source_text, arg1->source_text));
    e->prim.kind = prim;
    e->prim.arg_exprs[0] = arg0;
    e->prim.arg_exprs[1] = arg1;
    return e;
}


static struct expr *parse_symbol(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_IDENT);
    struct expr *e = expr_create(ctx, EXPR_SYM, ctx->token_text);
    e->sym.name = ctx->token_text;
    e->sym.name_hash = slice_hash_fnv1a(ctx->token_text);
    NEXT_TOKEN();
    return e;
}

static struct expr *parse_block(struct parse_ctx *ctx) {
    struct expr *first = parse_expr(ctx);
    if (ctx->token != TOK_SEMICOLON) {
        return first;
    }
    NEXT_TOKEN();
    if (ctx->token == TOK_KW_END || ctx->token == TOK_KW_ELIF || ctx->token == TOK_KW_ELSE || ctx->token == TOK_END) {
        return first;
    }
    struct expr *second = parse_block(ctx);
    if (!second) {
        return first;
    }
    return prim_create_bin(ctx, PRIM_SEQ, first, second);
}

static struct expr *parse_begin(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_BEGIN);
    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    struct expr *body = parse_block(ctx);
    if (ctx->token != TOK_KW_END) {
        PARSE_ERR("expected 'end' after 'begin' block");
    }
    NEXT_TOKEN();
    struct expr *e = expr_create(ctx, EXPR_BLOCK, slice_span(first_token, ctx->prev_token_text));
    e->block.body_expr = body;
    return e;
}

static struct expr *parse_def(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_PUB || ctx->token == TOK_KW_CONST || ctx->token == TOK_KW_VAR);
    unsigned int flags = 0;
    if (ctx->token == TOK_KW_PUB) {
        flags |= EXPR_FLAG_DEF_PUB;
    } else if (ctx->token == TOK_KW_VAR) {
        flags |= EXPR_FLAG_DEF_VAR;
    }

    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    
    struct expr *name_expr;
    struct expr *type_expr = NULL;
    struct expr *value_expr = NULL;

    if (ctx->token == TOK_KW_STATIC) {
        flags |= EXPR_FLAG_DEF_STATIC;
        NEXT_TOKEN();
    }
    if (ctx->token != TOK_IDENT) {
        PARSE_ERR("expected identifier");
    }
    name_expr = parse_symbol(ctx);
    if (ctx->token == TOK_COLON) {
        NEXT_TOKEN();
        type_expr = parse_expr(ctx);
    }
    if (ctx->token == TOK_ASSIGN) {
        NEXT_TOKEN();
        value_expr = parse_expr(ctx);
    }

    struct expr *e = expr_create(ctx, EXPR_DEF, slice_span(first_token, ctx->prev_token_text));
    e->flags = flags;
    e->def.name_expr = name_expr;
    e->def.type_expr = type_expr;
    e->def.value_expr = value_expr;
    return e;
}

static struct expr_decl *parse_decls(struct parse_ctx *ctx) {
    struct expr_decl *d;

    if (ctx->token != TOK_IDENT) {
        return NULL;
    }

    d = calloc(1, sizeof(struct expr_decl));
    d->name_expr = parse_symbol(ctx);

    if (ctx->token == TOK_COMMA) {
        NEXT_TOKEN();
        if (ctx->token != TOK_IDENT) {
            PARSE_ERR("unexpected identifier after ',' in declaration");
        }
        d->next = parse_decls(ctx);
        d->type_expr = d->next->type_expr;
        d->value_expr = d->next->value_expr;
        d->is_static = d->next->is_static;
    }
    else {
        if (ctx->token == TOK_COLON) {
            NEXT_TOKEN();
            if (ctx->token == TOK_KW_STATIC) {
                d->is_static = true;
                NEXT_TOKEN();
            }
            d->type_expr = parse_expr(ctx);
        }

        if (ctx->token == TOK_ASSIGN) {
            NEXT_TOKEN();
            d->value_expr = parse_expr(ctx);
        }

        if (ctx->token == TOK_SEMICOLON) {
            NEXT_TOKEN();
            d->next = parse_decls(ctx);
        }
    }

    return d;
}

struct expr *parse_module(char *source_text, struct error_ctx *err_ctx) {
    struct parse_ctx parse_ctx = { { 0 }, };
    parse_ctx.scan_ctx.cursor = source_text;
    parse_ctx.err_ctx = err_ctx;
    return do_parse_module(&parse_ctx);
}

struct expr *do_parse_module(struct parse_ctx *ctx) {
    slice_t first_token;

    if (setjmp(ctx->error_jmp_buf)) {
        return NULL;
    }

    NEXT_TOKEN();
    first_token = ctx->token_text;

    struct expr *body = parse_block(ctx);

    if (ctx->token != TOK_END) {
        PARSE_ERR("unexpected token in module");
    }

    struct expr *result = expr_create(ctx, EXPR_STRUCT, slice_span(first_token, ctx->prev_token_text));
    result->struc.body_expr = body;
    return result;
}

struct expr *parse_struct(struct parse_ctx *ctx) {
    slice_t first_token = ctx->token_text;
    
    assert(ctx->token == TOK_KW_STRUCT);
    NEXT_TOKEN();

    struct expr *body = parse_block(ctx);

    if (ctx->token != TOK_KW_END) {
        PARSE_ERR("expected 'end' after struct definition");
    }
    NEXT_TOKEN();

    struct expr *result = expr_create(ctx, EXPR_STRUCT, slice_span(first_token, ctx->prev_token_text));
    result->struc.body_expr = body;
    return result;
}

static struct expr *parse_fun(struct parse_ctx *ctx) {
    struct expr *result, *return_type_expr = NULL, *body_expr = NULL;
    struct expr_decl *params = NULL, *p;
    slice_t first_token = ctx->token_text;

    assert(ctx->token == TOK_KW_FUN);
    NEXT_TOKEN();

    if (ctx->token == TOK_LPAREN) {
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            params = parse_decls(ctx);
            if (ctx->token != TOK_RPAREN) {
                PARSE_ERR("expected ')' after parameter list");
            }
            NEXT_TOKEN();
        }
    }

    if (ctx->token == TOK_COLON) {
        NEXT_TOKEN();
        return_type_expr = parse_expr(ctx);
    }

    if (ctx->token == TOK_RARROW) {
        NEXT_TOKEN();
        body_expr = parse_expr(ctx);

        if (ctx->token == TOK_KW_END) {
            NEXT_TOKEN();
        }
    }

    if (!body_expr) {
        for (p = params; p; p = p->next) {
            if (!p->type_expr) {
                PARSE_ERR("incomplete parameter types in preceding function type definition");
            }
        }
        if (!return_type_expr) {
            PARSE_ERR("missing return type in preceding function type definition");
        }
    }

    result = expr_create(ctx, EXPR_FUN, slice_span(first_token, ctx->prev_token_text));
    result->fun.params = params;
    result->fun.return_type_expr = return_type_expr;
    result->fun.body_expr = body_expr;
    return result;
}

static struct expr *parse_if(struct parse_ctx *ctx, bool is_elif) {
    struct expr *result, *pred_expr, *then_expr, *else_expr;
    slice_t first_token = ctx->token_text;

    assert(is_elif ? ctx->token == TOK_KW_ELIF : ctx->token == TOK_KW_IF);
    NEXT_TOKEN();

    pred_expr = parse_expr(ctx);

    if (ctx->token != TOK_KW_THEN) {
        PARSE_ERR("expected 'then' after if condition");
    }
    NEXT_TOKEN();

    then_expr = parse_block(ctx);

    if (ctx->token == TOK_KW_ELSE) {
        NEXT_TOKEN();
        else_expr = parse_block(ctx);
        if (ctx->token != TOK_KW_END) {
            PARSE_ERR("expected 'end' to terminate 'if'");
        }
        NEXT_TOKEN();
    } else if (ctx->token == TOK_KW_ELIF) {
        else_expr = parse_if(ctx, true);
    } else if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
        slice_t text = then_expr->source_text;
        text.ptr += text.len;
        text.len = 0;
        else_expr = unit_create(ctx, text);
    } else {
        PARSE_ERR("expected 'elif', 'else' or 'end' to terminate");
    }

    result = expr_create(ctx, EXPR_COND, slice_span(first_token, else_expr->source_text));
    result->cond.pred_expr = pred_expr;
    result->cond.then_expr = then_expr;
    result->cond.else_expr = else_expr;
    return result;
}

static struct expr *parse_while(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_WHILE);
    return NULL;
}


static struct expr *parse_unary(struct parse_ctx *ctx, int prim) {
    struct expr *result;
    struct expr *arg;
    slice_t first_token = ctx->token_text;

    NEXT_TOKEN();

    arg = parse_atom(ctx);

    result = expr_create(ctx, EXPR_PRIM, slice_span(first_token, arg->source_text));
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = arg;
    return result;
}

static struct expr_link *parse_args(struct parse_ctx *ctx) {
    struct expr_link *a;
    
    if (ctx->token == TOK_RPAREN) {
        NEXT_TOKEN();
        return NULL;
    }

    a = calloc(1, sizeof(struct expr_link));
    a->expr = parse_expr(ctx);

    if (ctx->token == TOK_COMMA) {
        NEXT_TOKEN();
        a->next = parse_args(ctx);
    }
    else {
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            PARSE_ERR("expected ')' after argument list");
        }
    }

    return a;
}

static struct expr *parse_call(struct parse_ctx *ctx, struct expr *callable_expr) {
    struct expr *result;
    struct expr_link *args;

    assert(ctx->token == TOK_LPAREN);
    NEXT_TOKEN();

    args = parse_args(ctx);

    result = expr_create(ctx, EXPR_CALL, slice_span(callable_expr->source_text, ctx->prev_token_text));
    result->call.callable_expr = callable_expr;
    result->call.args = args;
    return result;
}

static struct expr *parse_prim_call(struct parse_ctx *ctx, int prim, uint arg_count) {
    struct expr *result;
    struct expr_link *args;
    slice_t name = ctx->token_text;
    uint i;

    NEXT_TOKEN();
    if (ctx->token != TOK_LPAREN) {
        PARSE_ERR("expected '(' after '%.*s'", name.len, name.ptr);
    }
    NEXT_TOKEN();

    args = parse_args(ctx);
    if (expr_list_length(args) != arg_count) {
        PARSE_ERR("expected %d arguments for '%.*s'", arg_count, name.len, name.ptr);
    }

    result = expr_create(ctx, EXPR_PRIM, slice_span(name, ctx->prev_token_text));
    result->prim.kind = prim;
    for (i = 0; i < arg_count; ++i, args = args->next) {
        result->prim.arg_exprs[i] = args->expr;
    }

    return result;
}

static struct expr *parse_single_arg_prim(struct parse_ctx *ctx, int prim) {
    struct expr *result, *e;
    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    if (ctx->token != TOK_LPAREN) {
        PARSE_ERR("expected '(' after 'expr'");
    }
    NEXT_TOKEN();
    e = parse_expr(ctx);
    if (ctx->token != TOK_RPAREN) {
        PARSE_ERR("expected ')' after 'expr' expression");
    }
    NEXT_TOKEN();
    result = expr_create(ctx, EXPR_PRIM, slice_span(first_token, ctx->prev_token_text));
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = e;
    return result;
}

static struct expr *parse_int(struct parse_ctx *ctx, int offset, int base) {
    struct expr *result = expr_create(ctx, EXPR_CONST, ctx->token_text);
    result->c.tag = &type_int;
    result->c.integer = (int)my_strtoll(ctx->token_text.ptr + offset, NULL, base);
    NEXT_TOKEN();
    return result;
}

static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence) {
    struct expr *lhs = parse_atom(ctx);

    while (true) {
        int prim = -1, left_assoc = 1, precedence, next_min_precedence;
        struct expr *rhs;

        switch (ctx->token) {
        case TOK_KW_OR:     prim = PRIM_LOGI_OR;     precedence = 1; break;

        case TOK_KW_AND:    prim = PRIM_LOGI_AND;    precedence = 2; break;

        case TOK_OR_BW:     prim = PRIM_BITWISE_OR;  precedence = 3; break;

        case TOK_XOR_BW:    prim = PRIM_BITWISE_XOR; precedence = 4; break;

        case TOK_AND_BW:    prim = PRIM_BITWISE_AND; precedence = 5; break;

        case TOK_EQ:        prim = PRIM_EQ;          precedence = 6; break;
        case TOK_NEQ:       prim = PRIM_NEQ;         precedence = 6; break;

        case TOK_LT:        prim = PRIM_LT;          precedence = 7; break;
        case TOK_GT:        prim = PRIM_GT;          precedence = 7; break;
        case TOK_LTEQ:      prim = PRIM_LTEQ;        precedence = 7; break;
        case TOK_GTEQ:      prim = PRIM_GTEQ;        precedence = 7; break;

        case TOK_LSH:       prim = PRIM_BITWISE_LSH; precedence = 8; break;
        case TOK_RSH:       prim = PRIM_BITWISE_RSH; precedence = 8; break;

        case TOK_PLUS:      prim = PRIM_ADD;         precedence = 9; break;
        case TOK_MINUS:     prim = PRIM_SUB;         precedence = 9; break;

        case TOK_MUL:       prim = PRIM_MUL;         precedence = 10; break;
        case TOK_DIV:       prim = PRIM_DIV;         precedence = 10; break;
        case TOK_MOD:       prim = PRIM_MOD;         precedence = 10; break;
        }

        if (prim < 0 || precedence < min_precedence) {
            break;
        }

        NEXT_TOKEN();

        next_min_precedence = precedence + (left_assoc ? 1 : 0);
        rhs = parse_infix(ctx, next_min_precedence);

        lhs = prim_create_bin(ctx, prim, lhs, rhs);
    }

    return lhs;
}

static void wrap_args_in_expr(struct parse_ctx *ctx, struct expr_link *arg) {
    struct expr *e;
    if (!arg) {
        return;
    }
    e = expr_create(ctx, EXPR_PRIM, arg->expr->source_text);
    e->prim.kind = PRIM_QUOTE;
    e->prim.arg_exprs[0] = arg->expr;
    arg->expr = e;
    wrap_args_in_expr(ctx, arg->next);
}

static struct expr *macroify(struct parse_ctx *ctx, struct expr *call_expr) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, call_expr->source_text);
    e->prim.kind = PRIM_SPLICE;
    e->prim.arg_exprs[0] = call_expr;
    wrap_args_in_expr(ctx, call_expr->call.args);
    return e;
}


static struct expr *parse_atom(struct parse_ctx *ctx) {
    struct expr *result;
    slice_t first_token = ctx->token_text;

    switch (ctx->token) {
    case TOK_LPAREN:
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            result = unit_create(ctx, slice_span(first_token, ctx->token_text));
        }
        else {
            result = parse_expr(ctx);
            if (ctx->token != TOK_RPAREN) {
                PARSE_ERR("expected ')'");
            }
        }
        NEXT_TOKEN();
        break;
    case TOK_IDENT:
        if (!slice_str_cmp(ctx->token_text, "assert")) {
            return parse_prim_call(ctx, PRIM_ASSERT, 1);
        }
        if (!slice_str_cmp(ctx->token_text, "quote")) {
            return parse_single_arg_prim(ctx, PRIM_QUOTE);
        }
        if (!slice_str_cmp(ctx->token_text, "splice")) {
            return parse_single_arg_prim(ctx, PRIM_SPLICE);
        }
        if (!slice_str_cmp(ctx->token_text, "print")) {
            return parse_single_arg_prim(ctx, PRIM_PRINT);
        }
        result = parse_symbol(ctx);
        break;
    case TOK_PLUS: return parse_unary(ctx, PRIM_PLUS);
    case TOK_MINUS: return parse_unary(ctx, PRIM_NEGATE);
    case TOK_KW_NOT: return parse_unary(ctx, PRIM_LOGI_NOT);
    case TOK_NOT_BW: return parse_unary(ctx, PRIM_BITWISE_NOT);
    case TOK_KW_TRUE: NEXT_TOKEN(); return bool_create(ctx, true, first_token);
    case TOK_KW_FALSE: NEXT_TOKEN(); return bool_create(ctx, false, first_token);
    case TOK_BIN: return parse_int(ctx, 2, 2);
    case TOK_OCT: return parse_int(ctx, 0, 8);
    case TOK_DEC: return parse_int(ctx, 0, 10);
    case TOK_HEX: return parse_int(ctx, 2, 16);
    case TOK_KW_STRUCT: return parse_struct(ctx);
    case TOK_KW_FUN: return parse_fun(ctx);
    case TOK_KW_PUB:
    case TOK_KW_CONST:
    case TOK_KW_VAR: return parse_def(ctx);
    case TOK_KW_IF: return parse_if(ctx, false);
    case TOK_KW_WHILE: return parse_while(ctx);
    case TOK_KW_STATIC: return parse_single_arg_prim(ctx, PRIM_STATIC);
    case TOK_KW_BEGIN: return parse_begin(ctx);
    default:
        PARSE_ERR("unexpected token in expression");
    }

    if (ctx->token == TOK_NOT) {
        NEXT_TOKEN();
        if (ctx->token != TOK_LPAREN) {
            PARSE_ERR("expected '(' after postfix '!'");
        }
        result = parse_call(ctx, result);
        result = macroify(ctx, result);
    }

    while (ctx->token == TOK_LPAREN) {
        result = parse_call(ctx, result);
    }

    return result;
}

static struct expr *parse_expr(struct parse_ctx *ctx) {
    return parse_infix(ctx, 1);
}
