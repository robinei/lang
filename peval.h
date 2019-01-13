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

struct scope {
    struct expr_link *closure_syms;

    struct scope *outer_scope;
    struct scope *nearest_function_scope;

    struct expr_decl *decls;
    struct expr_decl **last_decl_ptr;

    uint depth;
    enum scope_kind kind;
};

struct peval_ctx {
    struct error_ctx *err_ctx;
    struct module_ctx *mod_ctx;

    struct scope root_scope;
    struct scope *scope;

    uint force_full_expansion;

    uint assert_count;
    uint assert_fails;
    jmp_buf error_jmp_buf;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
