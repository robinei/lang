#include "mod.h"
#include "expr.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

void module_ctx_init(struct module_ctx *ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->err_ctx = err_ctx;
    pointer_table_init(&ctx->functions, 16);
    slice_table_init(&ctx->symbol_table, 16);
}

struct symbol *intern_slice(struct module_ctx *ctx, slice_t name) {
    struct symbol *result;
    if (!slice_table_get(&ctx->symbol_table, name, (void **)&result)) {
        result = malloc(sizeof(struct symbol) + name.len + 1);
        result->length = name.len;
        memcpy(result->data, name.ptr, name.len);
        result->data[name.len] = '\0';
        slice_table_put(&ctx->symbol_table, name, result);
    }
    return result;
}

struct symbol *intern_string(struct module_ctx *ctx, char *str) {
    return intern_slice(ctx, slice_from_str(str));
}
