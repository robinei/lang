#ifndef PARSE_H
#define PARSE_H

#include "scan.h"

typedef unsigned int uint;

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;


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
    EXPR_ARG,
    EXPR_FN,
    EXPR_LET,
    EXPR_STRUCT,
    EXPR_IF,
    EXPR_UNOP,
    EXPR_BINOP,
    EXPR_CALL,
};

#define BINDING_LIST_MAX 100

struct binding_list {
    uint count;
    slice_t *names;
    struct expr **types;
    struct expr **exprs;
};

struct expr;

struct expr_lit {
    slice_t value;
};

struct expr_sym {
    slice_t name;
};

struct expr_arg {
    uint index;
};

struct expr_fn {
    struct binding_list params;
    struct expr *return_type;
    struct expr *body;
};

struct expr_let {
    struct binding_list bindings;
    struct expr *body;
};

struct expr_struct {
    struct binding_list fields;
};

struct expr_if {
    struct expr *cond_expr;
    struct expr *then_expr;
    struct expr *else_expr;
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
    struct expr *fn_expr;
    struct expr **arg_exprs;
    uint arg_count;
};

struct expr {
    uint expr;
    union {
        struct expr_lit lit;
        struct expr_sym sym;
        struct expr_arg arg;
        struct expr_fn fn;
        struct expr_let let;
        struct expr_struct _struct;
        struct expr_if _if;
        struct expr_unop unop;
        struct expr_binop binop;
        struct expr_call call;
    } u;
};


#define ERROR_MAX 512

struct parse_ctx {
    struct scan_ctx scan;
    char *text;
    int token;
    slice_t token_text;

    char error[ERROR_MAX + 1];
};

struct expr *parse_module(struct parse_ctx *ctx);

#endif