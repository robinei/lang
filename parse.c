#include "parse.h"
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>

long long int my_strtoll(const char *nptr, const char **endptr, int base);
unsigned long long my_strtoull(const char *str, char **endptr, int base);
double my_strtod(const char *string, const char **endPtr);

#define LOWEST_PRECEDENCE 1
#define HIGHEST_PRECEDENCE 12
static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence);


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
    struct expr *e = allocate(ctx->arena, sizeof(struct expr));
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
static struct expr *string_create(struct parse_ctx *ctx, slice_t source_text) {
    assert(source_text.len > 2);
    assert(source_text.ptr[0] == '"');
    assert(source_text.ptr[source_text.len - 1] == '"');
    // TODO: handle escape sequences
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    source_text.ptr += 1;
    source_text.len -= 2;
    e->c.tag = &type_string;
    e->c.string = source_text;
    return e;
}
static struct expr *unit_create(struct parse_ctx *ctx, slice_t source_text) {
    struct expr *e = expr_create(ctx, EXPR_CONST, source_text);
    e->c.tag = &type_unit;
    return e;
}
static struct expr *prim_create_bin(struct parse_ctx *ctx, int prim, struct expr *lhs, struct expr *rhs) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, slice_span(lhs->source_text, rhs->source_text));
    e->prim.kind = prim;
    e->prim.arg_exprs[0] = lhs;
    e->prim.arg_exprs[1] = rhs;
    return e;
}


static struct expr *parse_expr(struct parse_ctx *ctx) {
    return parse_infix(ctx, LOWEST_PRECEDENCE + 1); /* disallow '=' operator */
}

static struct expr *parse_expr_seq(struct parse_ctx *ctx) {
    struct expr *first = parse_infix(ctx, LOWEST_PRECEDENCE); /* allow '=' operator */
    if (ctx->token != TOK_SEMICOLON) {
        return first;
    }
    NEXT_TOKEN();
    if (ctx->token == TOK_KW_END || ctx->token == TOK_KW_ELIF || ctx->token == TOK_KW_ELSE || ctx->token == TOK_END) {
        return first;
    }
    struct expr *rest = parse_expr_seq(ctx);
    if (!rest) {
        return first;
    }
    return prim_create_bin(ctx, PRIM_SEQ, first, rest);
}

static struct expr *maybe_wrap_in_block(struct parse_ctx *ctx, struct expr *e, slice_t first_token) {
    if (e->kind == EXPR_BLOCK) {
        return e; /* avoid redundant block in block */
    }
    if (e->kind != EXPR_DEF && (e->kind != EXPR_PRIM || e->prim.kind != PRIM_SEQ)) {
        return e; /* avoid redundant wrapping of single non-def expression */
    }
    struct expr *result = expr_create(ctx, EXPR_BLOCK, slice_span(first_token, ctx->prev_token_text));
    result->block.body_expr = e;
    return result;
}

static struct expr *parse_expr_seq_as_block(struct parse_ctx *ctx) {
    slice_t first_token = ctx->token_text;
    struct expr *body = parse_expr_seq(ctx);
    return maybe_wrap_in_block(ctx, body, first_token);
}

static struct expr *parse_block(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_BEGIN);
    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    struct expr *body = parse_expr_seq(ctx);
    if (ctx->token != TOK_KW_END) {
        PARSE_ERR("expected 'end' after 'begin' block");
    }
    NEXT_TOKEN();
    return maybe_wrap_in_block(ctx, body, first_token);
}

static struct expr *parse_symbol(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_IDENT);
    struct expr *e = expr_create(ctx, EXPR_SYM, ctx->token_text);
    e->sym = intern_slice(&ctx->global_ctx->symbol_table, ctx->token_text);
    NEXT_TOKEN();
    return e;
}

static struct expr *parse_def(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_CONST || ctx->token == TOK_KW_VAR);
    unsigned int flags = 0;
    if (ctx->token == TOK_KW_VAR) {
        flags |= EXPR_FLAG_DEF_VAR;
    }

    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    
    struct expr *type_expr = NULL;
    struct expr *value_expr = NULL;

    if (ctx->token == TOK_KW_STATIC) {
        flags |= EXPR_FLAG_DEF_STATIC;
        NEXT_TOKEN();
    }
    if (ctx->token != TOK_IDENT) {
        PARSE_ERR("expected identifier");
    }
    struct expr *name_expr = parse_symbol(ctx);
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

    d = allocate(ctx->arena, sizeof(struct expr_decl));
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

static struct expr *do_parse_module(struct parse_ctx *ctx) {
    if (setjmp(ctx->error_jmp_buf)) {
        return NULL;
    }

    NEXT_TOKEN();
    slice_t first_token = ctx->token_text;

    struct expr *body = parse_expr_seq(ctx);

    if (ctx->token != TOK_END) {
        PARSE_ERR("unexpected token in module");
    }

    struct expr *result = expr_create(ctx, EXPR_STRUCT, slice_span(first_token, ctx->prev_token_text));
    result->struc.body_expr = body;
    return result;
}

struct expr *parse_module(struct module_ctx *mod_ctx, slice_t source_text) {
    struct parse_ctx parse_ctx = {0};
    parse_ctx.arena = &mod_ctx->arena.a;
    parse_ctx.scan_ctx.cursor = source_text.ptr;
    parse_ctx.global_ctx = mod_ctx->global_ctx;
    parse_ctx.mod_ctx = mod_ctx;
    parse_ctx.err_ctx = &mod_ctx->err_ctx;
    return do_parse_module(&parse_ctx);
}

struct expr *parse_struct(struct parse_ctx *ctx) {
    slice_t first_token = ctx->token_text;
    
    assert(ctx->token == TOK_KW_STRUCT);
    NEXT_TOKEN();

    struct expr *body = parse_expr_seq(ctx);

    if (ctx->token != TOK_KW_END) {
        PARSE_ERR("expected 'end' after struct definition");
    }
    NEXT_TOKEN();

    struct expr *result = expr_create(ctx, EXPR_STRUCT, slice_span(first_token, ctx->prev_token_text));
    result->struc.body_expr = body;
    return result;
}

static struct expr *parse_fun(struct parse_ctx *ctx) {
    struct expr *return_type_expr = NULL, *body_expr = NULL;
    struct expr_decl *params = NULL;
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
        for (struct expr_decl *p = params; p; p = p->next) {
            if (!p->type_expr) {
                PARSE_ERR("incomplete parameter types in preceding function type definition");
            }
        }
        if (!return_type_expr) {
            PARSE_ERR("missing return type in preceding function type definition");
        }
    }

    struct expr *result = expr_create(ctx, EXPR_FUN, slice_span(first_token, ctx->prev_token_text));
    result->fun.params = params;
    result->fun.return_type_expr = return_type_expr;
    result->fun.body_expr = body_expr;
    return result;
}

static struct expr *parse_if(struct parse_ctx *ctx, bool is_elif) {
    slice_t first_token = ctx->token_text;

    assert(is_elif ? ctx->token == TOK_KW_ELIF : ctx->token == TOK_KW_IF);
    NEXT_TOKEN();

    struct expr *pred_expr = parse_expr(ctx);

    if (ctx->token != TOK_KW_THEN) {
        PARSE_ERR("expected 'then' after if condition");
    }
    NEXT_TOKEN();

    struct expr *then_expr = parse_expr_seq_as_block(ctx);

    struct expr *else_expr;
    if (ctx->token == TOK_KW_ELSE) {
        NEXT_TOKEN();
        else_expr = parse_expr_seq_as_block(ctx);
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

    struct expr *result = expr_create(ctx, EXPR_COND, slice_span(first_token, else_expr->source_text));
    result->cond.pred_expr = pred_expr;
    result->cond.then_expr = then_expr;
    result->cond.else_expr = else_expr;
    return result;
}

static struct expr *parse_while(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_WHILE);
    return NULL;
}

static struct expr_link *parse_args(struct parse_ctx *ctx) {
    struct expr_link *a;
    
    if (ctx->token == TOK_RPAREN) {
        NEXT_TOKEN();
        return NULL;
    }

    a = allocate(ctx->arena, sizeof(struct expr_link));
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

static struct expr *parse_prim_call(struct parse_ctx *ctx, int prim, uint arg_count) {
    slice_t name = ctx->token_text;

    NEXT_TOKEN();
    if (ctx->token != TOK_LPAREN) {
        PARSE_ERR("expected '(' after '%.*s'", name.len, name.ptr);
    }
    NEXT_TOKEN();

    struct expr_link *args = parse_args(ctx);
    if (expr_list_length(args) != arg_count) {
        PARSE_ERR("expected %d arguments for '%.*s'", arg_count, name.len, name.ptr);
    }

    struct expr *result = expr_create(ctx, EXPR_PRIM, slice_span(name, ctx->prev_token_text));
    result->prim.kind = prim;
    for (uint i = 0; i < arg_count; ++i, args = args->next) {
        result->prim.arg_exprs[i] = args->expr;
    }

    return result;
}

static struct expr *parse_single_arg_prim(struct parse_ctx *ctx, int prim) {
    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    if (ctx->token != TOK_LPAREN) {
        PARSE_ERR("expected '(' after 'expr'");
    }
    NEXT_TOKEN();
    struct expr *e = parse_expr(ctx);
    if (ctx->token != TOK_RPAREN) {
        PARSE_ERR("expected ')' after 'expr' expression");
    }
    NEXT_TOKEN();
    struct expr *result = expr_create(ctx, EXPR_PRIM, slice_span(first_token, ctx->prev_token_text));
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = e;
    return result;
}

static struct expr *parse_int(struct parse_ctx *ctx, int offset, int base) {
    struct expr *result = expr_create(ctx, EXPR_CONST, ctx->token_text);
    char *end = NULL;
    // TODO: parse negative signed numbers as such here rather than parsing unary negation of positive number
    uint64_t value = my_strtoull(ctx->token_text.ptr + offset, &end, base);
    assert(end == ctx->token_text.ptr + ctx->token_text.len);
    if (value <= INT64_MAX) {
        result->c.tag = &type_int;
        result->c.integer = (int64_t)value;
    } else {
        result->c.tag = &type_uint;
        result->c.uinteger = value;
    }
    NEXT_TOKEN();
    return result;
}

static struct expr *parse_real(struct parse_ctx *ctx) {
    struct expr *result = expr_create(ctx, EXPR_CONST, ctx->token_text);
    const char *end = NULL;
    result->c.tag = &type_real;
    result->c.real = my_strtod(ctx->token_text.ptr, &end);
    assert(end == ctx->token_text.ptr + ctx->token_text.len);
    NEXT_TOKEN();
    return result;
}

static struct expr *parse_unary(struct parse_ctx *ctx, int prim) {
    slice_t first_token = ctx->token_text;
    NEXT_TOKEN();
    struct expr *arg = parse_infix(ctx, HIGHEST_PRECEDENCE);
    struct expr *result = expr_create(ctx, EXPR_PRIM, slice_span(first_token, arg->source_text));
    result->prim.kind = prim;
    result->prim.arg_exprs[0] = arg;
    return result;
}

static struct expr *parse_atom(struct parse_ctx *ctx) {
    struct expr *result;
    slice_t first_token = ctx->token_text;

    switch (ctx->token) {
    case TOK_LPAREN:
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            result = unit_create(ctx, slice_span(first_token, ctx->token_text));
        } else {
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
    case TOK_REAL: return parse_real(ctx);
    case TOK_STRING: NEXT_TOKEN(); return string_create(ctx, first_token);
    case TOK_KW_STRUCT: return parse_struct(ctx);
    case TOK_KW_SELF: NEXT_TOKEN(); return expr_create(ctx, EXPR_SELF, first_token);
    case TOK_KW_FUN: return parse_fun(ctx);
    case TOK_KW_CONST:
    case TOK_KW_VAR: return parse_def(ctx);
    case TOK_KW_IF: return parse_if(ctx, false);
    case TOK_KW_WHILE: return parse_while(ctx);
    case TOK_KW_IMPORT: return parse_single_arg_prim(ctx, PRIM_IMPORT);
    case TOK_KW_STATIC: return parse_single_arg_prim(ctx, PRIM_STATIC);
    case TOK_KW_BEGIN: return parse_block(ctx);
    default:
        PARSE_ERR("unexpected token in expression");
    }

    return result;
}

static struct expr *parse_call(struct parse_ctx *ctx, struct expr *callable_expr) {
    assert(ctx->token == TOK_LPAREN);
    NEXT_TOKEN();

    struct expr_link *args = parse_args(ctx);

    struct expr *result = expr_create(ctx, EXPR_CALL, slice_span(callable_expr->source_text, ctx->prev_token_text));
    result->call.callable_expr = callable_expr;
    result->call.args = args;
    return result;
}

static void quote_args(struct parse_ctx *ctx, struct expr_link *arg) {
    if (!arg) {
        return;
    }
    struct expr *e = expr_create(ctx, EXPR_PRIM, arg->expr->source_text);
    e->prim.kind = PRIM_QUOTE;
    e->prim.arg_exprs[0] = arg->expr;
    arg->expr = e;
    quote_args(ctx, arg->next);
}

static struct expr *macroify(struct parse_ctx *ctx, struct expr *call_expr) {
    struct expr *e = expr_create(ctx, EXPR_PRIM, call_expr->source_text);
    e->prim.kind = PRIM_SPLICE;
    e->prim.arg_exprs[0] = call_expr;
    quote_args(ctx, call_expr->call.args);
    return e;
}

#define HANDLE_INFIX_PRIM(Precedence, Prim, LeftAssoc) \
    { \
        if (Precedence < min_precedence) { return result; } \
        NEXT_TOKEN(); \
        struct expr *rhs = parse_infix(ctx, Precedence + LeftAssoc); \
        result = prim_create_bin(ctx, Prim, result, rhs); \
        continue; \
    }

static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence) {
    struct expr *result = parse_atom(ctx);

    while (true) {
        switch (ctx->token) {
        case TOK_ASSIGN:    HANDLE_INFIX_PRIM(LOWEST_PRECEDENCE, PRIM_ASSIGN, 1)

        case TOK_KW_OR:     HANDLE_INFIX_PRIM(2, PRIM_LOGI_OR, 1)

        case TOK_KW_AND:    HANDLE_INFIX_PRIM(3, PRIM_LOGI_AND, 1)

        case TOK_OR_BW:     HANDLE_INFIX_PRIM(4, PRIM_BITWISE_OR, 1)

        case TOK_XOR_BW:    HANDLE_INFIX_PRIM(5, PRIM_BITWISE_XOR, 1)

        case TOK_AND_BW:    HANDLE_INFIX_PRIM(6, PRIM_BITWISE_AND, 1)

        case TOK_EQ:        HANDLE_INFIX_PRIM(7, PRIM_EQ, 1)
        case TOK_NEQ:       HANDLE_INFIX_PRIM(7, PRIM_NEQ, 1)

        case TOK_LT:        HANDLE_INFIX_PRIM(8, PRIM_LT, 1)
        case TOK_GT:        HANDLE_INFIX_PRIM(8, PRIM_GT, 1)
        case TOK_LTEQ:      HANDLE_INFIX_PRIM(8, PRIM_LTEQ, 1)
        case TOK_GTEQ:      HANDLE_INFIX_PRIM(8, PRIM_GTEQ, 1)

        case TOK_LSH:       HANDLE_INFIX_PRIM(9, PRIM_BITWISE_LSH, 1)
        case TOK_RSH:       HANDLE_INFIX_PRIM(9, PRIM_BITWISE_RSH, 1)

        case TOK_PLUS:      HANDLE_INFIX_PRIM(10, PRIM_ADD, 1)
        case TOK_MINUS:     HANDLE_INFIX_PRIM(10, PRIM_SUB, 1)

        case TOK_MUL:       HANDLE_INFIX_PRIM(11, PRIM_MUL, 1)
        case TOK_DIV:       HANDLE_INFIX_PRIM(11, PRIM_DIV, 1)
        case TOK_MOD:       HANDLE_INFIX_PRIM(11, PRIM_MOD, 1)

        case TOK_PERIOD:    HANDLE_INFIX_PRIM(HIGHEST_PRECEDENCE, PRIM_DOT, 1)
        case TOK_LPAREN:
            if (HIGHEST_PRECEDENCE < min_precedence) { return result; }
            result = parse_call(ctx, result);
            continue;
        case TOK_NOT:
            if (HIGHEST_PRECEDENCE < min_precedence) { return result; }
            NEXT_TOKEN();
            if (ctx->token != TOK_LPAREN) {
                PARSE_ERR("expected '(' after postfix '!'");
            }
            result = parse_call(ctx, result);
            result = macroify(ctx, result);
            continue;

        default: return result;
        }
    }
}
