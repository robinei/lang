#ifndef PEVAL_H
#define PEVAL_H

#include "expr.h"
#include "mod.h"
#include <setjmp.h>

enum scope_kind {
    SCOPE_STRUCT,
    SCOPE_FUNCTION,
    SCOPE_BLOCK
};

struct scope {
    struct scope *outer_scope;

    struct expr_decl *decls;
    struct expr_decl **last_decl_ptr;

    struct type *self;

    uint free_var_count;
    enum scope_kind kind;
};

struct peval_ctx {
    struct arena *arena;
    struct error_ctx *err_ctx;
    struct module_ctx *mod_ctx;

    struct scope root_scope;
    struct scope *scope;

    uint force_full_expansion;

    jmp_buf error_jmp_buf;

    struct symbol *sym_lambda;
};

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx);

struct expr *peval(struct peval_ctx *ctx, struct expr *e);

#endif
