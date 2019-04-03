#include "global.h"
#include <string.h>

void global_ctx_init(struct global_ctx *global_ctx) {
    memset(global_ctx, 0, sizeof(struct global_ctx));
    symbol_table_init(&global_ctx->symbol_table, &global_ctx->arena);
}
