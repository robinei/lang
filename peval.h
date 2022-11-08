#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "mod.h"
#include <setjmp.h>

struct peval_ctx {
    struct allocator *arena;
    struct global_ctx *global_ctx;
    struct module_ctx *mod_ctx;
    struct error_ctx *err_ctx;

    struct scope *scope;

    uint force_full_expansion;

    jmp_buf error_jmp_buf;

    struct symbol *sym_lambda;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
