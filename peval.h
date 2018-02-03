#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "mod.h"
#include <setjmp.h>

struct peval_ctx {
    struct error_ctx *err_ctx;
    struct module_ctx *mod_ctx;

    struct scope *scope;
    uint closed_var_count;
    slice_t closest_name;

    bool identify_closures;
    bool force_type_expansion;
    uint force_full_expansion;
    bool inhibit_call_expansion;

    uint assert_count;
    uint assert_fails;
    jmp_buf error_jmp_buf;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
