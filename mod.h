#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"
#include "slice.h"

struct function {
    struct symbol *name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct arena arena;
    struct error_ctx *err_ctx;
    struct pointer_table functions;
    struct symbol_table symbol_table;
    struct expr *struct_expr;
    struct type *module_type;
};

void module_ctx_init(struct module_ctx *ctx, struct error_ctx *err_ctx);

#endif
