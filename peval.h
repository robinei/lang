#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "slice.h"
#include <setjmp.h>

#define NAME_STACK_SIZE 16

struct peval_ctx {
    struct module *mod;

    struct slice_table symbols;

    uint binding_count;
    uint binding_capacity;
    struct peval_binding *bindings;

    slice_t closest_name;
    slice_t name_stack[NAME_STACK_SIZE];
    uint name_stack_count;

    uint force_full_expansion;
    uint inhibit_call_expansion;

    jmp_buf error_jmp_buf;
};

struct module {
    struct slice_table functions;
    struct expr *struct_expr;
};

struct module *partial_eval_module(struct peval_ctx *ctx, struct expr *e);

#endif
