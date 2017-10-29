#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

static struct expr *expr_create(struct parse_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}

static void expr_print(struct expr *e, int indent) {
    switch (e->expr) {
    case EXPR_BINOP:
        ;
    }
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
}

#define NEXT_TOKEN() \
    do { \
        ctx->token = scan_next_token(&ctx->scan, &ctx->token_text.ptr); \
        ctx->token_text.len = (int)(ctx->scan.cursor - ctx->token_text.ptr); \
        printf("%s: '%.*s'\n", token_strings[ctx->token], (int)(ctx->scan.cursor - ctx->token_text.ptr), ctx->token_text.ptr); \
    } while (0)


static struct expr *parse_expr(struct parse_ctx *ctx);
static struct expr *parse_atom(struct parse_ctx *ctx);

static int token_ends_expr(int tok) {
    switch (tok) {
    case TOK_END:
    case TOK_RPAREN:
    case TOK_RBRACKET:
    case TOK_SEMICOLON:
    case TOK_COLON:
    case TOK_COMMA:
    case TOK_KW_IN:
    case TOK_KW_ELIF:
    case TOK_KW_ELSE:
        return 1;
    }
    return 0;
}

static struct expr *parse_unary(struct parse_ctx *ctx, int unop) {
    struct expr *arg, *result;

    NEXT_TOKEN();
    arg = parse_atom(ctx);
    if (!arg) {
        return NULL;
    }

    result = expr_create(ctx, EXPR_UNOP);
    result->u.unop.arg = arg;
    return result;
}

static struct expr *parse_call(struct parse_ctx *ctx, struct expr *fn_expr) {
    struct expr *arg_exprs[FN_MAX_PARAM];
    int arg_count = 0;
    struct expr *result;

    assert(ctx->token == TOK_LPAREN);
    NEXT_TOKEN();

    if (ctx->token == TOK_RPAREN) {
        NEXT_TOKEN();
    }
    else {
        while (1) {
            arg_exprs[arg_count] = parse_expr(ctx);
            if (!arg_exprs[arg_count]) {
                return NULL;
            }
            ++arg_count;
            if (ctx->token == TOK_COMMA) {
                NEXT_TOKEN();
                continue;
            }
            if (ctx->token == TOK_RPAREN) {
                NEXT_TOKEN();
                break;
            }
            parse_error(ctx, "unexpected token while parsing function call argument list");
            return NULL;
        }
    }

    result = expr_create(ctx, EXPR_CALL);
    result->u.call.fn_expr = fn_expr;
    result->u.call.arg_count = arg_count;
    result->u.call.arg_exprs = malloc(sizeof(struct expr *) * arg_count);
    memcpy(result->u.call.arg_exprs, arg_exprs, sizeof(struct expr *) * arg_count);
    return result;
}

static struct expr *parse_atom(struct parse_ctx *ctx) {
    struct expr *result;

    switch (ctx->token) {
    case TOK_LPAREN:
        NEXT_TOKEN();
        result = parse_expr(ctx);
        if (!result) {
            return NULL;
        }
        if (ctx->token != TOK_RPAREN) {
            parse_error(ctx, "expected ')'");
            return NULL;
        }
        NEXT_TOKEN();
        break;
    case TOK_PLUS: return parse_unary(ctx, UNOP_PLUS);
    case TOK_MINUS: return parse_unary(ctx, UNOP_NEGATE);
    case TOK_NOT: return parse_unary(ctx, UNOP_LOGI_NOT);
    case TOK_NOT_BW: return parse_unary(ctx, UNOP_BITWISE_NOT);
    case TOK_KW_INT:
    case TOK_IDENT:
        result = expr_create(ctx, EXPR_SYM);
        result->u.sym.name = ctx->token_text;
        NEXT_TOKEN();
        break;
    case TOK_BIN:
    case TOK_OCT:
    case TOK_DEC:
    case TOK_HEX:
        result = expr_create(ctx, EXPR_LIT);
        result->u.lit.value = ctx->token_text;
        NEXT_TOKEN();
        break;
    default:
        parse_error(ctx, "unexpected token in expression");
        return NULL;
    }

    while (1) {
        if (ctx->token == TOK_LPAREN) {
            result = parse_call(ctx, result);
            if (!result) {
                return NULL;
            }
        }
        else {
            break;
        }
    }

    return result;
}

static struct expr *parse_infix(struct parse_ctx *ctx, int min_precedence) {
    struct expr *lhs = parse_atom(ctx);
    if (!lhs) {
        return NULL;
    }

    while (1) {
        int op = -1, left_assoc = 1, precedence, next_min_precedence;
        struct expr *rhs, *temp;

        switch (ctx->token) {
        case TOK_OR:        op = BINOP_LOGI_OR;     precedence = 1; break;

        case TOK_AND:       op = BINOP_LOGI_AND;    precedence = 2; break;

        case TOK_OR_BW:     op = BINOP_BITWISE_OR;  precedence = 3; break;

        case TOK_XOR_BW:    op = BINOP_BITWISE_XOR; precedence = 4; break;

        case TOK_AND_BW:    op = BINOP_BITWISE_AND; precedence = 5; break;

        case TOK_EQ:        op = BINOP_EQ;          precedence = 6; break;
        case TOK_NEQ:       op = BINOP_NEQ;         precedence = 6; break;

        case TOK_LT:        op = BINOP_LT;          precedence = 7; break;
        case TOK_GT:        op = BINOP_GT;          precedence = 7; break;
        case TOK_LTEQ:      op = BINOP_LTEQ;        precedence = 7; break;
        case TOK_GTEQ:      op = BINOP_GTEQ;        precedence = 7; break;

        case TOK_LSH:       op = BINOP_BITWISE_LSH; precedence = 8; break;
        case TOK_RSH:       op = BINOP_BITWISE_RSH; precedence = 8; break;

        case TOK_PLUS:      op = BINOP_ADD;         precedence = 9; break;
        case TOK_MINUS:     op = BINOP_SUB;         precedence = 9; break;

        case TOK_MUL:       op = BINOP_MUL;         precedence = 10; break;
        case TOK_DIV:       op = BINOP_DIV;         precedence = 10; break;
        case TOK_MOD:       op = BINOP_MOD;         precedence = 10; break;
        }

        if (op < 0 || precedence < min_precedence) {
            break;
        }

        NEXT_TOKEN();

        next_min_precedence = precedence + (left_assoc ? 1 : 0);
        rhs = parse_infix(ctx, next_min_precedence);
        if (!rhs) {
            return NULL;
        }

        temp = expr_create(ctx, EXPR_BINOP);
        temp->u.binop.binop = op;
        temp->u.binop.lhs = lhs;
        temp->u.binop.rhs = rhs;
        lhs = temp;
    }

    return lhs;
}

static struct expr *parse_fn(struct parse_ctx *ctx) {
    slice_t param_names[FN_MAX_PARAM];
    struct expr *param_types[FN_MAX_PARAM];
    int param_count = 0, orig_count, i;
    struct expr *result, *type, *return_type = NULL, *body;

    assert(ctx->token == TOK_KW_FN);
    NEXT_TOKEN();

    if (ctx->token == TOK_LPAREN) {
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            while (1) {
                orig_count = param_count;

                while (1) {
                    if (param_count == FN_MAX_PARAM) {
                        parse_error(ctx, "too many fn params (%d)", param_count);
                        return NULL;
                    }
                    if (ctx->token != TOK_IDENT) {
                        parse_error(ctx, "expected identifier in parameter list");
                        return NULL;
                    }
                    param_names[param_count++] = ctx->token_text;
                    NEXT_TOKEN();
                    if (ctx->token == TOK_COMMA) {
                        NEXT_TOKEN();
                        continue;
                    }
                    if (ctx->token == TOK_COLON) {
                        NEXT_TOKEN();
                        break;
                    }
                    parse_error(ctx, "unexpected token in parameter list");
                    return NULL;
                }

                type = parse_atom(ctx);
                if (!type) {
                    return NULL;
                }

                for (i = orig_count; i < param_count; ++i) {
                    param_types[i] = type;
                }

                if (ctx->token == TOK_SEMICOLON) {
                    NEXT_TOKEN();
                    continue;
                }
                if (ctx->token == TOK_RPAREN) {
                    NEXT_TOKEN();
                    break;
                }
                parse_error(ctx, "unexpected token in parameter list");
                return NULL;
            }
        }
    }

    if (ctx->token != TOK_COLON) {
        return_type = parse_atom(ctx);
        if (!return_type) {
            return NULL;
        }
    }

    if (ctx->token != TOK_COLON) {
        parse_error(ctx, "expected ':' after 'fn' parameter list");
        return NULL;
    }

    NEXT_TOKEN();

    body = parse_expr(ctx);
    if (!body) {
        return NULL;
    }

    result = expr_create(ctx, EXPR_FN);
    result->u.fn.param_count = param_count;
    result->u.fn.param_names = malloc(sizeof(slice_t) * param_count);
    result->u.fn.param_types = malloc(sizeof(struct expr *) * param_count);
    memcpy(result->u.fn.param_names, param_names, sizeof(slice_t) * param_count);
    memcpy(result->u.fn.param_types, param_types, sizeof(struct expr *) * param_count);
    result->u.fn.return_type = return_type;
    result->u.fn.body = body;
    return result;
}

static struct expr *parse_let(struct parse_ctx *ctx) {
    slice_t binding_names[LET_MAX_BINDING];
    struct expr *binding_types[LET_MAX_BINDING];
    struct expr *binding_exprs[LET_MAX_BINDING];
    int binding_count = 0;
    struct expr *result, *body;

    assert(ctx->token == TOK_KW_LET);

    while (1) {
        if (binding_count == LET_MAX_BINDING) {
            parse_error(ctx, "too many let bindings (%d)", binding_count);
            return NULL;
        }
        NEXT_TOKEN();
        if (ctx->token != TOK_IDENT) {
            parse_error(ctx, "expected identifier in let expression");
            return NULL;
        }
        binding_names[binding_count] = ctx->token_text;
        binding_types[binding_count] = NULL;
        NEXT_TOKEN();
        if (ctx->token == TOK_COLON) {
            NEXT_TOKEN();
            binding_types[binding_count] = parse_atom(ctx);
            if (!binding_types[binding_count]) {
                return NULL;
            }
        }
        if (ctx->token != TOK_ASSIGN) {
            parse_error(ctx, "expected '=' in let expression");
            return NULL;
        }
        NEXT_TOKEN();
        binding_exprs[binding_count] = parse_expr(ctx);
        if (!binding_exprs[binding_count]) {
            return NULL;
        }
        ++binding_count;
        if (ctx->token == TOK_SEMICOLON) {
            continue;
        }
        if (ctx->token == TOK_KW_IN) {
            NEXT_TOKEN();
            break;
        }
        parse_error(ctx, "unexpected token while parsing 'let' form");
        return NULL;
    }

    body = parse_expr(ctx);
    if (!body) {
        return NULL;
    }

    result = expr_create(ctx, EXPR_LET);
    result->u.let.binding_count = binding_count;
    result->u.let.binding_names = malloc(sizeof(slice_t) * binding_count);
    result->u.let.binding_types = malloc(sizeof(struct expr *) * binding_count);
    result->u.let.binding_exprs = malloc(sizeof(struct expr *) * binding_count);
    memcpy(result->u.let.binding_names, binding_names, sizeof(slice_t) * binding_count);
    memcpy(result->u.let.binding_types, binding_types, sizeof(struct expr *) * binding_count);
    memcpy(result->u.let.binding_exprs, binding_exprs, sizeof(struct expr *) * binding_count);

    return result;
}

static struct expr *parse_if(struct parse_ctx *ctx, int is_elif) {
    struct expr *result, *cond_expr, *then_expr, *else_expr;

    assert(is_elif ? ctx->token == TOK_KW_ELIF : ctx->token == TOK_KW_IF);
    NEXT_TOKEN();

    cond_expr = parse_expr(ctx);
    if (!cond_expr) {
        return NULL;
    }
    if (ctx->token != TOK_COLON) {
        parse_error(ctx, "expected ':' after if condition");
        return NULL;
    }
    NEXT_TOKEN();

    then_expr = parse_expr(ctx);
    if (!then_expr) {
        return NULL;
    }

    if (ctx->token == TOK_KW_ELSE) {
        NEXT_TOKEN();
        else_expr = parse_expr(ctx);
        if (!else_expr) {
            return NULL;
        }
    }
    else if (ctx->token == TOK_KW_ELIF) {
        else_expr = parse_if(ctx, 1);
        if (!else_expr) {
            return NULL;
        }
    }

    if (!is_elif && ctx->token == TOK_SEMICOLON) {
        NEXT_TOKEN();
    }

    result = expr_create(ctx, EXPR_IF);
    result->u._if.cond_expr = cond_expr;
    result->u._if.then_expr = then_expr;
    result->u._if.else_expr = else_expr;
    return result;
}

static struct expr *parse_while(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_WHILE);
    return NULL;
}

static struct expr *parse_expr(struct parse_ctx *ctx) {
    struct expr *first, *second, *e;

    switch (ctx->token) {
    case TOK_KW_FN:
        return parse_fn(ctx);
    case TOK_KW_LET:
        return parse_let(ctx);
    case TOK_KW_IF:
        return parse_if(ctx, 0);
    case TOK_KW_WHILE:
        return parse_while(ctx);
    }

    first = parse_infix(ctx, 1);
    if (!first) {
        return NULL;
    }

    if (token_ends_expr(ctx->token)) {
        return first;
    }

    second = parse_expr(ctx);
    if (!second) {
        return NULL;
    }

    e = expr_create(ctx, EXPR_BINOP);
    e->u.binop.binop = BINOP_SEQ;
    e->u.binop.lhs = first;
    e->u.binop.rhs = second;

    return e;
}

int parse_module(struct parse_ctx *ctx) {
    struct expr *e;

    NEXT_TOKEN();

    if (ctx->token != TOK_KW_MODULE) {
        parse_error(ctx, "expected 'module' at top of file");
        return 0;
    }
    NEXT_TOKEN();
    if (ctx->token != TOK_IDENT) {
        parse_error(ctx, "expected identifier after 'module'");
        return 0;
    }
    NEXT_TOKEN();
    if (ctx->token != TOK_SEMICOLON) {
        parse_error(ctx, "expected ';' after 'module' declaration");
        return 0;
    }
    NEXT_TOKEN();

    if (ctx->token == TOK_KW_IMPORT) {
        while (1) {
            NEXT_TOKEN();
            if (ctx->token != TOK_IDENT) {
                parse_error(ctx, "expected identifier in import list");
                return 0;
            }
            NEXT_TOKEN();
            if (ctx->token == TOK_COMMA) {
                continue;
            }
            if (ctx->token == TOK_SEMICOLON) {
                NEXT_TOKEN();
                break;
            }
            parse_error(ctx, "unexpected token in import list");
            return 0;
        }
    }

    while (1) {
        if (ctx->token == TOK_IDENT) {
            NEXT_TOKEN();
            if (ctx->token != TOK_ASSIGN) {
                parse_error(ctx, "expected '=' after top level identifier");
                return 0;
            }
            NEXT_TOKEN();
            e = parse_expr(ctx);
            if (!e) {
                return 0;
            }
            if (ctx->token != TOK_SEMICOLON) {
                parse_error(ctx, "expected ';' after top level expression");
                return 0;
            }
            NEXT_TOKEN();
            continue;
        }
        if (ctx->token == TOK_END) {
            break;
        }
        parse_error(ctx, "unexpected token at top level");
        return 0;
    }

    return 1;
}