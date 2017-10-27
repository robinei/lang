#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"

typedef unsigned int uint;

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;

static int slice_eq_str(slice_t s, const char *str) {
    int len = strlen(str);
    if (s.len != len) {
        return 0;
    }
    return !memcmp(s.ptr, str, len);
}

enum {
    UNOP_PLUS,
    UNOP_NEGATE,
    UNOP_LOGI_NOT,
    UNOP_BITWISE_NOT,
};

enum {
    BINOP_SEQ,

    BINOP_LOGI_OR,

    BINOP_LOGI_AND,

    BINOP_BITWISE_OR,

    BINOP_BITWISE_XOR,

    BINOP_BITWISE_AND,

    BINOP_EQ,
    BINOP_NEQ,

    BINOP_LT,
    BINOP_GT,
    BINOP_LTEQ,
    BINOP_GTEQ,

    BINOP_BITWISE_LSH,
    BINOP_BITWISE_RSH,

    BINOP_ADD,
    BINOP_SUB,

    BINOP_MUL,
    BINOP_DIV,
    BINOP_MOD,
};

enum {
    EXPR_LIT,
    EXPR_SYM,
    EXPR_FN,
    EXPR_UNOP,
    EXPR_BINOP,
    EXPR_CALL,
};

#define FN_MAX_PARAM 100

struct expr;

struct expr_lit {
    slice_t value;
};

struct expr_sym {
    slice_t value;
};

struct expr_fn {
    uint param_count;
    slice_t *param_names;
    struct expr *body;
};

struct expr_unop {
    uint unop;
    struct expr *arg;
};

struct expr_binop {
    uint binop;
    struct expr *lhs, *rhs;
};

struct expr_call {
    struct expr *func;
    struct expr **args;
    uint arg_count;
};

struct expr {
    uint expr;
    union {
        struct expr_lit lit;
        struct expr_sym sym;
        struct expr_fn fn;
        struct expr_unop unop;
        struct expr_binop binop;
        struct expr_call call;
    } u;
};


static struct expr *expr_create(struct parse_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}



#define ERROR_MAX 512

struct parse_ctx {
    struct lexer_ctx lexer;
    char *text;
    int token;
    slice_t token_text;

    char error[ERROR_MAX + 1];
};

static void parse_error(struct parse_ctx *ctx, const char *format, ...) {
    va_list args;
    int used = 0;
    char *line;
    int line_len;
    int err_line_offset;
    int err_len;
    int i;

    used += snprintf(ctx->error + used, ERROR_MAX - used, "(line %d) parse error: ", ctx->lexer.line + 1);
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
        ctx->token = lexer_next_token(&ctx->lexer, &ctx->token_text.ptr); \
        ctx->token_text.len = (int)(ctx->lexer.cursor - ctx->token_text.ptr); \
        printf("%s: '%.*s'\n", lexer_token_strings[ctx->token], (int)(ctx->lexer.cursor - ctx->token_text.ptr), ctx->token_text.ptr); \
    } while (0)


static struct expr *parse_expr(struct parse_ctx *ctx);
static struct expr *parse_atom(struct parse_ctx *ctx);


static struct expr *parse_unary(struct parse_ctx *ctx, int unop) {
    struct expr *arg, *e = NULL;

    NEXT_TOKEN();
    arg = parse_atom(ctx);
    if (arg) {
        e = expr_create(ctx, EXPR_UNOP);
        e->u.unop.arg = arg;
    }
    return e;
}

static struct expr *parse_atom(struct parse_ctx *ctx) {
    struct expr *e = NULL;

    switch (ctx->token) {
    case TOK_LPAREN:
        NEXT_TOKEN();
        e = parse_expr(ctx);
        if (!e) {
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
    case TOK_IDENT:
        e = expr_create(ctx, EXPR_SYM);
        e->u.sym.value = ctx->token_text;
        NEXT_TOKEN();
        break;
    case TOK_BIN:
    case TOK_OCT:
    case TOK_DEC:
    case TOK_HEX:
        e = expr_create(ctx, EXPR_LIT);
        e->u.lit.value = ctx->token_text;
        NEXT_TOKEN();
        break;
    default:
        parse_error(ctx, "unexpected token in expression");
        return NULL;
    }

    return e;
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
    int num_params = 0;
    struct expr *type, *body, *fn;

    assert(ctx->token == TOK_KW_FN);
    NEXT_TOKEN();

    if (ctx->token == TOK_LPAREN) {
        NEXT_TOKEN();
        if (ctx->token == TOK_RPAREN) {
            NEXT_TOKEN();
        }
        else {
            while (1) {
                while (1) {
                    if (ctx->token != TOK_IDENT) {
                        parse_error(ctx, "expected identifier in parameter list");
                        return NULL;
                    }
                    param_names[num_params++] = ctx->token_text;
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

                type = parse_expr(ctx);
                if (!type) {
                    parse_error(ctx, "expected type expression");
                    return NULL;
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


        if (ctx->token != TOK_COLON) {
            type = parse_expr(ctx);
            if (!type) {
                parse_error(ctx, "expected return type expression or ':'");
                return NULL;
            }
        }

        if (ctx->token != TOK_COLON) {
            parse_error(ctx, "expected ':' after 'fn' parameter list");
            return NULL;
        }

        NEXT_TOKEN();
    }
    else {
        NEXT_TOKEN();
        if (ctx->token != TOK_COLON) {
            parse_error(ctx, "expected '(' or ':' after 'fn'");
            return NULL;
        }
    }

    body = parse_expr(ctx);
    if (!body) {
        parse_error(ctx, "expected function body expression");
        return NULL;
    }

    fn = expr_create(ctx, EXPR_FN);
    fn->u.fn.param_count = num_params;
    fn->u.fn.param_names = malloc(sizeof(slice_t) * num_params);
    memcpy(fn->u.fn.param_names, param_names, sizeof(slice_t) * num_params);
    fn->u.fn.body = body;
    return fn;
}

static struct expr *parse_let(struct parse_ctx *ctx) {
    assert(ctx->token == TOK_KW_LET);
    return NULL;
}

static struct expr *parse_expr(struct parse_ctx *ctx) {
    struct expr *first, *second, *e;

    switch (ctx->token) {
    case TOK_KW_FN:
        return parse_fn(ctx);
    case TOK_KW_LET:
        return parse_let(ctx);
    }

    first = parse_infix(ctx, 1);
    if (!first) {
        return NULL;
    }

    switch (ctx->token) {
    case TOK_RPAREN:
    case TOK_RBRACKET:
    case TOK_SEMICOLON:
    case TOK_COLON:
    case TOK_COMMA:
    case TOK_KW_IN:
    case TOK_KW_ELIF:
    case TOK_KW_ELSE:
        /* no more expressions */
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

static int parse_module(struct parse_ctx *ctx) {
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

    while (ctx->token != TOK_END) {
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
        }
        parse_error(ctx, "unexpected token at top level");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    char *text =

        "module Test;\n"
        "import IO, String;\n"

        "Bar = fn() int: 1;"
        ;

    struct parse_ctx ctx = {0,};
    ctx.lexer.cursor = text;
    ctx.text = text;

    /*while (1) {
        char *begin;
        int tok = lexer_next_token(&ctx.lexer, &begin);
        printf("%s: '%.*s'\n", lexer_token_strings[tok], (int)(ctx.lexer.cursor - begin), begin);
        if (tok == TOK_ERR || tok == TOK_END) {
            break;
        }
    }
    printf("lines: %d\n", ctx.lexer.line);*/

    if (parse_module(&ctx)) {
        printf("OK\n");
    }
    else {
        printf("%s\n", ctx.error);
    }

    fgetc(stdin);
    return 0;
}