#ifndef GLOBAL_H
#define GLOBAL_H

#include "sym.h"

struct global_ctx {
    struct tracking_allocator alloc;
    struct arena_allocator arena;

    struct symbol_table symbol_table;
    struct slice_table modules;
};

void global_ctx_init(struct global_ctx *global_ctx);

#endif
