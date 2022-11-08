#ifndef MOD_H
#define MOD_H

#include "global.h"
#include "error.h"

#define SPECIALIZED_ARGS_HASH(A) const_args_hash(A, FNV_SEED)
#define SPECIALIZED_ARGS_EQ(A, B) const_args_eq(ctx, A, B)
#define SPECIALIZATIONS_TABLE(X) X(specializations_table, struct expr_link *, struct function *, SPECIALIZED_ARGS_HASH, SPECIALIZED_ARGS_EQ)
DECLARE_HASHTABLE(SPECIALIZATIONS_TABLE)

struct function {
    struct symbol *name;
    struct scope *env;
    struct expr *fun_expr;
    struct specializations_table specializations;
};

struct module_ctx {
    struct tracking_allocator alloc;
    struct arena_allocator arena;

    struct global_ctx *global_ctx;
    struct error_ctx err_ctx;

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
