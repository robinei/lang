#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"

struct function {
    slice_t name;
    struct expr *fun_expr;
    struct expr *free_var_syms;
    struct function *next;
};

struct module_ctx {
    struct error_ctx *err_ctx;
    struct slice_table functions;
    struct expr *struct_expr;
    uint closed_var_count;
};

void module_ctx_init(struct module_ctx *ctx, struct expr *struct_expr);

#endif
