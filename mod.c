#include "mod.h"
#include "expr.h"
#include <string.h>
#include <assert.h>

void module_ctx_init(struct module_ctx *ctx, struct expr *struct_expr) {
    assert(struct_expr->kind == EXPR_STRUCT);
    memset(ctx, 0, sizeof(*ctx));
    ctx->struct_expr = struct_expr;
    slice_table_init(&ctx->functions, 16);
}
