#ifndef MOD_H
#define MOD_H

#include "error.h"

struct module_ctx {
    struct slice_table functions;
    struct expr *struct_expr;
};

void module_ctx_init(struct module_ctx *ctx, struct expr *struct_expr);

#endif
