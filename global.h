#ifndef GLOBAL_H
#define GLOBAL_H

#include "sym.h"

#define MODULE_HASHTABLE(X) X(module_hashtable, slice_t, struct module_ctx *, slice_hash_fnv1a, slice_equals)
DECLARE_HASHTABLE(MODULE_HASHTABLE)

struct global_ctx {
    struct tracking_allocator alloc;
    struct arena_allocator arena;

    struct symbol_table symbol_table;
    struct module_hashtable modules;
};

void global_ctx_init(struct global_ctx *global_ctx);

#endif
