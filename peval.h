#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "mod.h"
#include <setjmp.h>

enum scope_kind {
    SCOPE_STATIC,
    SCOPE_FUNCTION,
    SCOPE_LOCAL
};

struct binding {
    slice_t name;
    struct expr *expr;
    struct scope *scope;
    uint name_hash;
    bool pevaled;
};

struct scope {
    struct expr *pending_dummy_funs;
    struct function *pending_functions;
    struct expr *closure_syms;

    struct scope *outer_scope;
    struct scope *nearest_function_scope;

    struct binding *bindings;
    uint num_bindings;
    uint max_bindings;
    bool stack_bindings;

    uint depth;
    enum scope_kind kind;
};

struct peval_ctx {
    struct error_ctx *err_ctx;
    struct module_ctx *mod_ctx;

    struct scope root_scope;
    struct scope *scope;
    uint closed_var_count;
    slice_t closest_name;

    bool identify_closures;
    uint force_full_expansion;
    bool inhibit_call_expansion;

    uint assert_count;
    uint assert_fails;
    jmp_buf error_jmp_buf;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
