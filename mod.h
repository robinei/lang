#ifndef MOD_H
#define MOD_H

#include "global.h"
#include "expr.h"
#include "error.h"

struct function {
    struct symbol *name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct global_ctx *global_ctx;
    struct arena arena;
    struct error_ctx err_ctx;

    struct pointer_table functions;

    slice_t source_text; /* buffer is owned */
    struct expr *struct_expr; /* parsed representation */
    struct arena_mark parse_mark; /* arena mark for when parsing is done */

    struct type *module_type;

    uint total_assert_count;
    uint asserts_hit;
    uint asserts_failed;
};

struct module_ctx *module_load(struct global_ctx *global_ctx, struct module_ctx *mod_ctx, slice_t path);
void module_free(struct module_ctx *mod_ctx);

#endif
