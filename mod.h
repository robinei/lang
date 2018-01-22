#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"

struct function {
    slice_t name;
    struct function *next;

    struct expr *fn_expr; /* original expr */
    struct expr_decl *params;
    struct expr *return_type_expr;
    struct expr *body_expr;
};

struct module_ctx {
    struct error_ctx *err_ctx;
    struct slice_table functions;
    struct expr *struct_expr;
    uint closed_var_count;
};

void module_ctx_init(struct module_ctx *ctx, struct expr *struct_expr);

#endif
