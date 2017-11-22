#ifndef EXPR_H
#define EXPR_H

#include "type.h"
#include "slice.h"
#include "prim.h"


#define FOR_ALL_EXPRS(X) \
    X(CONST) \
    X(SYM) \
    X(FN) \
    X(LET) \
    X(STRUCT) \
    X(IF) \
    X(PRIM) \
    X(CALL)

#define DECL_EXPR_ENUM(name) EXPR_##name,
enum {
    FOR_ALL_EXPRS(DECL_EXPR_ENUM)
};
#undef DECL_EXPR_ENUM

extern const char *expr_names[];



struct expr_decl {
    slice_t name;
    struct expr *type_expr;
    struct expr *value_expr;
    struct expr_decl *next;
};

struct expr_const {
    struct type *type;
    union {
        int _bool;
        int _int;
        struct type *type;
        slice_t fn_name;
    } u;
};

struct expr_sym {
    slice_t name;
};

struct expr_fn {
    uint param_count;
    struct expr_fn_param *params;
    struct expr *return_type_expr;
    struct expr *body_expr; /* NULL for fn type declaration */
};
struct expr_fn_param {
    slice_t name;
    struct expr *type_expr;
    struct expr_fn_param *next;
};

struct expr_let {
    uint binding_count;
    struct expr_decl *bindings;
    struct expr *body_expr;
};

struct expr_struct {
    uint field_count;
    struct expr_decl *fields;
};

struct expr_if {
    struct expr *cond_expr;
    struct expr *then_expr;
    struct expr *else_expr;
};

struct expr_prim {
    uint prim;
    uint arg_count;
    struct expr *arg_expr0;
    struct expr *arg_expr1;
};

struct expr_call {
    struct expr *fn_expr;
    uint arg_count;
    struct expr_call_arg *args;
};
struct expr_call_arg {
    struct expr *expr;
    struct expr_call_arg *next;
};

struct expr {
    uint expr;
    union {
        struct expr_const _const;
        struct expr_sym sym;
        struct expr_fn fn;
        struct expr_let let;
        struct expr_struct _struct;
        struct expr_if _if;
        struct expr_prim prim;
        struct expr_call call;
    } u;
};


struct print_ctx {
    uint indent;
};

void print_expr(struct print_ctx *ctx, struct expr *e);

extern struct expr expr_true;
extern struct expr expr_false;

#endif
