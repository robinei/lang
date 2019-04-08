#include "global.h"
#include <string.h>

void global_ctx_init(struct global_ctx *global_ctx) {
    memset(global_ctx, 0, sizeof(struct global_ctx));
    tracking_allocator_init(&global_ctx->alloc, default_allocator);
    arena_allocator_init(&global_ctx->arena, &global_ctx->alloc.a, DEFAULT_ARENA_BUFFER_SIZE);
    slice_table_init(&global_ctx->modules, &global_ctx->alloc.a, 0);
    symbol_table_init(&global_ctx->symbol_table, &global_ctx->arena.a, &global_ctx->alloc.a);
}
