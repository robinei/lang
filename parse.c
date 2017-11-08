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
}

#define NEXT_TOKEN() \
    do { \
        ctx->token = scan_next_token(&ctx->scan, &ctx->token_text.ptr); \
        ctx->token_text.len = (int)(ctx->scan.cursor - ctx->token_text.ptr); \
        printf("%s: '%.*s'\n", token_strings[ctx->token], (int)(ctx->scan.cursor - ctx->token_text.ptr), ctx->token_text.ptr); \
    } while (0)


static int token_ends_expr(int tok) {
    switch (tok) {
    case TOK_END:
    case TOK_RPAREN:
    case TOK_RBRACKET:
    case TOK_RBRACE:
    case TOK_SEMICOLON:
    case TOK_COLON:
    case TOK_COMMA:
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

struct expr *parse_module(struct parse_ctx *ctx) {
    uint field_count = 0;
    struct expr_struct_field *first_field = NULL, *last_field = NULL, *f;
    struct expr *result;

    NEXT_TOKEN();

    while (ctx->token != TOK_END) {
        if (ctx->token == TOK_IDENT) {
            f = calloc(1, sizeof(struct expr_struct_field));
            if (last_field) { last_field->next = f; }
            else { first_field = f; }
            last_field = f;

            f->name = ctx->token_text;
            ++field_count;
            NEXT_TOKEN();

            if (ctx->token == TOK_COLON) {
                NEXT_TOKEN();
                f->type_expr = parse_expr(ctx);
                if (!f->type_expr) {
                    return NULL;
                }
            }

            if (ctx->token != TOK_ASSIGN) {
                parse_error(ctx, "expected '=' in top level definition");
                return NULL;
            }

            NEXT_TOKEN();
            f->value_expr = parse_expr(ctx);
            if (!f->value_expr) {
                return NULL;
            }

            if (ctx->token == TOK_SEMICOLON) {
                NEXT_TOKEN();
                continue;
            }

            parse_error(ctx, "expected ';' after top level definition");
            return NULL;
        }

        parse_error(ctx, "unexpected token while parsing top level definitions");
        return NULL;
    }

    result = expr_create(ctx, EXPR_STRUCT);
    result->u._struct.field_count = field_count;
    result->u._struct.fields = first_field;
    return result;
}

struct expr *parse_struct(struct parse_ctx *ctx) {
    uint field_count = 0;
    struct expr_struct_field *f, *temp,
        *first_field = NULL,
        *last_field = NULL,
        *last_valued = NULL,
        *last_typed = NULL;
    struct expr *result, *type, *value;

    assert(ctx->token == TOK_KW_STRUCT);
    NEXT_TOKEN();

    if (!token_ends_expr(ctx->token)) {
        while (1) {
            if (ctx->token != TOK_IDENT) {
                parse_error(ctx, "expected identifier in struct definition");
                return NULL;
            }

            f = calloc(1, sizeof(struct expr_struct_field));
            if (last_field) { last_field->next = f; }
            else { first_field = f; }
            last_field = f;

            f->name = ctx->token_text;
            ++field_count;
            NEXT_TOKEN();

            if (ctx->token == TOK_COMMA) {
                NEXT_TOKEN();
                continue;
            }

            if (ctx->token == TOK_COLON) {
                NEXT_TOKEN();

                type = parse_expr(ctx);
                if (!type) {
                    return NULL;
                }

                temp = last_typed ? last_typed->next : first_field;
                for (; temp; temp = temp->next) {
                    temp->type_expr = type;
                    last_typed = temp;
                }
            }

            if (ctx->token == TOK_ASSIGN) {
                NEXT_TOKEN();

                value = parse_expr(ctx);
                if (!value) {
                    return NULL;
                }

                temp = last_valued ? last_valued->next : first_field;
                for (; temp; temp = temp->next) {
                    temp->value_expr = value;
                    last_valued = temp;
                }
            }
            else if (!f->type_expr) {
                parse_error(ctx, "expected type or value for preceding struct field(s)");
                return NULL;
            }

            if (ctx->token == TOK_SEMICOLON) {
                NEXT_TOKEN();
                if (token_ends_expr(ctx->token)) {
                    break;
                }
                continue;
            }

            if (token_ends_expr(ctx->token)) {
                break;
            }

            parse_error(ctx, "unexpected token in struct definition");
            return NULL;
        }
    }


    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    result = expr_create(ctx, EXPR_STRUCT);
    result->u._struct.field_count = field_count;
    result->u._struct.fields = first_field;
    return result;
}

static struct expr *parse_fn(struct parse_ctx *ctx) {
    uint param_count = 0;
    struct expr_fn_param *p, *temp,
        *first_param = NULL,
        *last_param = NULL,
        *last_typed = NULL,
        *last_valued = NULL;
    struct expr *return_type = NULL, *body = NULL, *result, *type;

    assert(ctx->token == TOK_KW_FN);
    NEXT_TOKEN();

    if (ctx->token == TOK_LPAREN) {
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            while (1) {
                if (ctx->token != TOK_IDENT) {
                    parse_error(ctx, "expected identifier in parameter list");
                    return NULL;
                }

                p = calloc(1, sizeof(struct expr_fn_param));
                if (last_param) { last_param->next = p; }
                else { first_param = p; }
                last_param = p;

                p->name = ctx->token_text;
                ++param_count;
                NEXT_TOKEN();

                if (ctx->token == TOK_COMMA) {
                    NEXT_TOKEN();
                    continue;
                }

                if (ctx->token == TOK_COLON) {
                    NEXT_TOKEN();

                    type = parse_expr(ctx);
                    if (!type) {
                        return NULL;
                    }

                    temp = last_typed ? last_typed->next : first_param;
                    for (; temp; temp = temp->next) {
                        temp->type_expr = type;
                        last_typed = temp;
                    }
                }

                if (ctx->token == TOK_SEMICOLON) {
                    NEXT_TOKEN();
                    last_typed = p;
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

    if (ctx->token != TOK_COLON && !token_ends_expr(ctx->token)) {
        return_type = parse_expr(ctx);
        if (!return_type) {
            return NULL;
        }
    }

    if (ctx->token == TOK_COLON) {
        NEXT_TOKEN();

        body = parse_expr(ctx);
        if (!body) {
            return NULL;
        }
    }

    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    if (!body) {
        for (p = first_param; p; p = p->next) {
            if (!p->type_expr) {
                parse_error(ctx, "incomplete parameter types in preceding function type definition");
                return NULL;
            }
        }
        if (!return_type) {
            parse_error(ctx, "missing return type in preceding function type definition");
            return NULL;
        }
    }

    result = expr_create(ctx, EXPR_FN);
    result->u.fn.param_count = param_count;
    result->u.fn.params = first_param;
    result->u.fn.return_type_expr = return_type;
    result->u.fn.body_expr = body;
    return result;
}

static struct expr *parse_let(struct parse_ctx *ctx) {
    uint binding_count = 0;
    struct expr_let_binding *b, *first_binding = NULL, *last_binding = NULL;
    struct expr *result, *body;

    assert(ctx->token == TOK_KW_LET);

    while (1) {
        NEXT_TOKEN();
        if (ctx->token != TOK_IDENT) {
            parse_error(ctx, "expected identifier in let expression");
            return NULL;
        }

        b = calloc(1, sizeof(struct expr_let_binding));
        if (last_binding) { last_binding->next = b; }
        else { first_binding = b; }
        last_binding = b;

        b->name = ctx->token_text;
        ++binding_count;
        NEXT_TOKEN();

        if (ctx->token == TOK_COLON) {
            NEXT_TOKEN();
            b->type_expr = parse_expr(ctx);
            if (!b->type_expr) {
                return NULL;
            }
        }

        if (ctx->token != TOK_ASSIGN) {
            parse_error(ctx, "expected '=' in let expression");
            return NULL;
        }
        NEXT_TOKEN();

        b->value_expr = parse_expr(ctx);
        if (!b->value_expr) {
            return NULL;
        }

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

    if (ctx->token == TOK_KW_END) {
        NEXT_TOKEN();
    }

    result = expr_create(ctx, EXPR_LET);
    result->u.let.binding_count = binding_count;
    result->u.let.bindings = first_binding;
    result->u.let.body_expr = body;
    return result;
}

static struct expr *parse_if(struct parse_ctx *ctx, int is_elif) {
    struct expr *result, *cond_expr, *then_expr, *else_expr = NULL;

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

    if (!is_elif && ctx->token == TOK_KW_END) {
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


static struct expr *parse_unary(struct parse_ctx *ctx, int prim) {
    struct expr *e, *result;

    NEXT_TOKEN();
    e = parse_atom(ctx);
    if (!e) {
        return NULL;
    }

    result = expr_create(ctx, EXPR_PRIM);
    result->u.prim.prim = prim;
    result->u.prim.arg_count = 1;
    result->u.prim.arg_expr0 = e;
    return result;
}

static struct expr *parse_call(struct parse_ctx *ctx, struct expr *fn_expr) {
    int arg_count = 0;
    struct expr_call_arg *a, *first_arg = NULL, *last_arg = NULL;
    struct expr *result;

    assert(ctx->token == TOK_LPAREN);
    NEXT_TOKEN();

    if (ctx->token == TOK_RPAREN) {
        NEXT_TOKEN();
    }
    else {
        while (1) {
            a = calloc(1, sizeof(struct expr_call_arg));
            if (last_arg) { last_arg->next = a; }
            else { first_arg = a; }
            last_arg = a;

            ++arg_count;

            a->expr = parse_expr(ctx);
            if (!a->expr) {
                return NULL;
            }

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
    result->u.call.args = first_arg;
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
    if (!lhs) {
        return NULL;
    }

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
        if (!rhs) {
            return NULL;
        }

        temp = expr_create(ctx, EXPR_PRIM);
        temp->u.prim.prim = prim;
        temp->u.prim.arg_count = 2;
        temp->u.prim.arg_expr0 = lhs;
        temp->u.prim.arg_expr1 = rhs;
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
        if (!result) {
            return NULL;
        }
        if (ctx->token != TOK_RPAREN) {
            parse_error(ctx, "expected ')'");
            return NULL;
        }
        NEXT_TOKEN();
        break;
    case TOK_KW_TYPE:
    case TOK_KW_BOOL:
    case TOK_KW_INT:
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

static struct expr *parse_expr(struct parse_ctx *ctx) {
    struct expr *first, *second, *e;

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

    e = expr_create(ctx, EXPR_PRIM);
    e->u.prim.prim = PRIM_SEQ;
    e->u.prim.arg_count = 2;
    e->u.prim.arg_expr0 = first;
    e->u.prim.arg_expr1 = second;

    return e;
}
