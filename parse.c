#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

long long int my_strtoll(const char *nptr, const char **endptr, int base);
double my_strtod(const char *string, const char **endPtr);

static struct expr *expr_create(struct parse_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}

static void parse_error(struct parse_ctx *ctx, const char *format, ...) {
    va_list args;
    int used = 0;
    char *line;
    int line_len;
    int err_line_offset;
    int err_len;
    int i;

    used += snprintf(ctx->error + used, ERROR_MAX - used, "(line %d) parse error: ", ctx->scan.line + 1);
    if (used > ERROR_MAX) {
        used = ERROR_MAX;
    }

    va_start(args, format);
    used += vsnprintf(ctx->error + used, ERROR_MAX - used, format, args);
    va_end(args);
    if (used > ERROR_MAX) {
        used = ERROR_MAX;
    }

    line = ctx->token_text.ptr;
    while (line > ctx->text && line[-1] != '\n') {
        --line;
    }

    line_len = 0;
    while (line[line_len] && line[line_len] != '\n') {
        ++line_len;
    }

    if (used < ERROR_MAX) {
        ctx->error[used++] = '\n';
    }

    err_line_offset = (int)(ctx->token_text.ptr - line);
    for (i = 0; i < err_line_offset && used < ERROR_MAX; ++i) {
        ctx->error[used++] = ' ';
    }
    err_len = ctx->token_text.len;
    if (err_len <= 0) {
        err_len = 1;
    }
    for (i = 0; i < err_len && used < ERROR_MAX; ++i) {
        ctx->error[used++] = 'v';
    }

    used += snprintf(ctx->error + used, ERROR_MAX - used, "\n%.*s", line_len, line);
    if (used > ERROR_MAX) {
        used = ERROR_MAX;
    }

    ctx->error[used] = '\0';

    longjmp(ctx->error_jmp_buf, 1);
}

#define NEXT_TOKEN() \
    do { \
        ctx->token = scan_next_token(&ctx->scan, &ctx->token_text.ptr); \
        ctx->token_text.len = (int)(ctx->scan.cursor - ctx->token_text.ptr); \
    } while (0)
//printf("%s: '%.*s'\n", token_strings[ctx->token], (int)(ctx->scan.cursor - ctx->token_text.ptr), ctx->token_text.ptr);


static int token_ends_expr(int tok) {
    switch (tok) {
    case TOK_END:
    case TOK_RPAREN:
    case TOK_RBRACKET:
    case TOK_RBRACE:
    case TOK_SEMICOLON:
    case TOK_COLON:
    case TOK_COMMA:
    case TOK_ASSIGN:
    case TOK_KW_END:
    case TOK_KW_IN:
    case TOK_KW_OF:
    case TOK_KW_ELIF:
    case TOK_KW_ELSE:
        return 1;
    }
    return 0;
}

static struct expr *parse_expr(struct parse_ctx *ctx);
static struct expr *parse_atom(struct parse_ctx *ctx);

static struct expr_decl *parse_decls(struct parse_ctx *ctx) {
    struct expr_decl *f;

    if (ctx->token != TOK_IDENT) {
        return NULL;
    }

    f = calloc(1, sizeof(struct expr_decl));
    f->name = ctx->token_text;

    NEXT_TOKEN();

    if (ctx->token == TOK_COMMA) {
        NEXT_TOKEN();
        if (ctx->token != TOK_IDENT) {
            parse_error(ctx, "unexpected identifier after ',' in declaration");
        }
        f->next = parse_decls(ctx);
        f->type_expr = f->next->type_expr;
        f->value_expr = f->next->value_expr;
    }
    else {
        if (ctx->token == TOK_COLON) {
            NEXT_TOKEN();
            f->type_expr = parse_expr(ctx);
        }

        if (ctx->token == TOK_ASSIGN) {
            NEXT_TOKEN();
            f->value_expr = parse_expr(ctx);
        }

        if (ctx->token == TOK_SEMICOLON) {
            NEXT_TOKEN();
            f->next = parse_decls(ctx);
        }
    }

    return f;
}

struct expr *parse_module(struct parse_ctx *ctx) {
    struct expr *result;

    if (setjmp(ctx->error_jmp_buf)) {
        return NULL;
    }

    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_STRUCT);
    result->u._struct.fields = parse_decls(ctx);

    if (ctx->token != TOK_END) {
        parse_error(ctx, "unexpected token in module");
    }

    return result;
}

struct expr *parse_struct(struct parse_ctx *ctx) {
    struct expr *result;
    
    assert(ctx->token == TOK_KW_STRUCT);
    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_STRUCT);
    result->u._struct.fields = parse_decls(ctx);

    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }
    else if (!token_ends_expr(ctx->token)) {
        parse_error(ctx, "unexpected token after struct definition");
    }

    return result;
}

static struct expr *parse_fn(struct parse_ctx *ctx) {
    struct expr_decl *p;
    struct expr *result;

    assert(ctx->token == TOK_KW_FN);
    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_FN);

    if (ctx->token == TOK_LPAREN) {
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            result->u.fn.params = parse_decls(ctx);
            if (ctx->token != TOK_RPAREN) {
                parse_error(ctx, "expected ')' after parameter list");
            }
            NEXT_TOKEN();
        }
    }

    if (ctx->token != TOK_COLON && !token_ends_expr(ctx->token)) {
        result->u.fn.return_type_expr = parse_expr(ctx);
    }

    if (ctx->token == TOK_COLON) {
        NEXT_TOKEN();
        result->u.fn.body_expr = parse_expr(ctx);
    }

    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    if (!result->u.fn.body_expr) {
        for (p = result->u.fn.params; p; p = p->next) {
            if (!p->type_expr) {
                parse_error(ctx, "incomplete parameter types in preceding function type definition");
            }
        }
        if (!result->u.fn.return_type_expr) {
            parse_error(ctx, "missing return type in preceding function type definition");
        }
    }

    return result;
}

static struct expr *parse_let(struct parse_ctx *ctx) {
    struct expr *result;

    assert(ctx->token == TOK_KW_LET);
    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_LET);
    result->u.let.bindings = parse_decls(ctx);

    if (ctx->token != TOK_KW_IN) {
        parse_error(ctx, "expected 'in' after 'let' declarations");
    }
    NEXT_TOKEN();

    result->u.let.body_expr = parse_expr(ctx);

    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    return result;
}

static struct expr *parse_if(struct parse_ctx *ctx, int is_elif) {
    struct expr *result;

    assert(is_elif ? ctx->token == TOK_KW_ELIF : ctx->token == TOK_KW_IF);
    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_IF);
    result->u._if.cond_expr = parse_expr(ctx);

    if (ctx->token != TOK_COLON) {
        parse_error(ctx, "expected ':' after if condition");
    }
    NEXT_TOKEN();

    result->u._if.then_expr = parse_expr(ctx);

    if (ctx->token == TOK_KW_ELSE) {
        NEXT_TOKEN();
        result->u._if.else_expr = parse_expr(ctx);
    }
    else if (ctx->token == TOK_KW_ELIF) {
        result->u._if.else_expr = parse_if(ctx, 1);
    }

    if (!is_elif && ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    return result;
}

static struct expr *parse_while(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_WHILE);
    return NULL;
}


static struct expr *parse_unary(struct parse_ctx *ctx, int prim) {
    struct expr *result;

    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_PRIM);
    result->u.prim.prim = prim;
    result->u.prim.arg_exprs[0] = parse_atom(ctx);
    return result;
}

static struct expr_call_arg *parse_args(struct parse_ctx *ctx) {
    struct expr_call_arg *a;
    
    if (ctx->token == TOK_RPAREN) {
        NEXT_TOKEN();
        return NULL;
    }

    a = calloc(1, sizeof(struct expr_call_arg));
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
            parse_error(ctx, "expected ')' after argument list");
        }
    }

    return a;
}

static struct expr *parse_call(struct parse_ctx *ctx, struct expr *fn_expr) {
    struct expr *result;

    assert(ctx->token == TOK_LPAREN);
    NEXT_TOKEN();

    result = expr_create(ctx, EXPR_CALL);
    result->u.call.fn_expr = fn_expr;
    result->u.call.args = parse_args(ctx);
    return result;
}

static struct expr *parse_int(struct parse_ctx *ctx, int offset, int base) {
    struct expr *result = expr_create(ctx, EXPR_CONST);
    result->u._const.type = &type_int;
    result->u._const.u._int = (int)my_strtoll(ctx->token_text.ptr + offset, NULL, base);
    NEXT_TOKEN();
    return result;
}

static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence) {
    struct expr *lhs = parse_atom(ctx);

    while (1) {
        int prim = -1, left_assoc = 1, precedence, next_min_precedence;
        struct expr *rhs, *temp;

        switch (ctx->token) {
        case TOK_OR:        prim = PRIM_LOGI_OR;     precedence = 1; break;

        case TOK_AND:       prim = PRIM_LOGI_AND;    precedence = 2; break;

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

        temp = expr_create(ctx, EXPR_PRIM);
        temp->u.prim.prim = prim;
        temp->u.prim.arg_exprs[0] = lhs;
        temp->u.prim.arg_exprs[1] = rhs;
        lhs = temp;
    }

    return lhs;
}

static struct expr *parse_atom(struct parse_ctx *ctx) {
    struct expr *result;

    switch (ctx->token) {
    case TOK_LPAREN:
        NEXT_TOKEN();
        result = parse_expr(ctx);
        if (ctx->token != TOK_RPAREN) {
            parse_error(ctx, "expected ')'");
        }
        NEXT_TOKEN();
        break;
    case TOK_IDENT:
        result = expr_create(ctx, EXPR_SYM);
        result->u.sym.name = ctx->token_text;
        NEXT_TOKEN();
        break;
    case TOK_PLUS: return parse_unary(ctx, PRIM_PLUS);
    case TOK_MINUS: return parse_unary(ctx, PRIM_NEGATE);
    case TOK_NOT: return parse_unary(ctx, PRIM_LOGI_NOT);
    case TOK_NOT_BW: return parse_unary(ctx, PRIM_BITWISE_NOT);
    case TOK_KW_TRUE: NEXT_TOKEN(); return &expr_true;
    case TOK_KW_FALSE: NEXT_TOKEN(); return &expr_false;
    case TOK_BIN: return parse_int(ctx, 2, 2);
    case TOK_OCT: return parse_int(ctx, 0, 8);
    case TOK_DEC: return parse_int(ctx, 0, 10);
    case TOK_HEX: return parse_int(ctx, 2, 16);
    case TOK_KW_STRUCT: return parse_struct(ctx);
    case TOK_KW_FN: return parse_fn(ctx);
    case TOK_KW_LET: return parse_let(ctx);
    case TOK_KW_IF: return parse_if(ctx, 0);
    case TOK_KW_WHILE: return parse_while(ctx);
    default:
        parse_error(ctx, "unexpected token in expression");
    }

    while (1) {
        if (ctx->token == TOK_LPAREN) {
            result = parse_call(ctx, result);
        }
        else {
            break;
        }
    }

    return result;
}

static struct expr *parse_expr(struct parse_ctx *ctx) {
    struct expr *first, *second, *e;

    first = parse_infix(ctx, 1);

    if (token_ends_expr(ctx->token)) {
        return first;
    }

    second = parse_expr(ctx);

    e = expr_create(ctx, EXPR_PRIM);
    e->u.prim.prim = PRIM_SEQ;
    e->u.prim.arg_exprs[0] = first;
    e->u.prim.arg_exprs[1] = second;

    return e;
}
