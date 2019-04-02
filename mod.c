#include "mod.h"
#include "expr.h"
#include <string.h>

void module_ctx_init(struct module_ctx *ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->err_ctx = err_ctx;
    symbol_table_init(&ctx->symbol_table, &ctx->arena);
}
