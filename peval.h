#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "mod.h"
#include <setjmp.h>

#define NAME_STACK_SIZE 16

struct peval_ctx {
    struct error_ctx *err_ctx;
    struct module_ctx *mod_ctx;

    struct slice_table symbols;

    uint binding_count;
    uint binding_capacity;
    struct peval_binding *bindings;

    uint pending_fn_count;
    uint pending_fn_capacity;
    struct expr **pending_fns;

    slice_t closest_name;
    slice_t name_stack[NAME_STACK_SIZE];
    uint name_stack_count;

    uint force_full_expansion;
    uint inhibit_call_expansion;

    uint assert_count;
    uint assert_fails;
    jmp_buf error_jmp_buf;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
