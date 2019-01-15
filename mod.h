#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"

struct function {
    slice_t name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct error_ctx *err_ctx;
    struct slice_table functions;
    struct expr *struct_expr;
};

void module_ctx_init(struct module_ctx *ctx, struct expr *struct_expr);

#endif
