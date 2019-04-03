#ifndef GLOBAL_H
#define GLOBAL_H

#include "sym.h"

struct global_ctx {
    struct arena arena;
    struct symbol_table symbol_table;
};

void global_ctx_init(struct global_ctx *global_ctx);

#endif
