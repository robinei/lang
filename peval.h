#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "slice.h"
#include <setjmp.h>

struct peval_ctx {
    struct slice_table symbols;
    struct slice_table functions;

    uint binding_count;
    uint binding_capacity;
    struct peval_binding *bindings;

    jmp_buf error_jmp_buf;
};

struct expr *partial_eval_module(struct peval_ctx *ctx, struct expr *e);

#endif
