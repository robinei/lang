#include "error.h"
#include <stdlib.h>
#include <assert.h>

#define PEVAL_ERR(exp, ...) \
    do { \
        error_emit(ctx->err_ctx, ERROR_CATEGORY_ERROR, expr_source_text(exp), __VA_ARGS__); \
        longjmp(ctx->error_jmp_buf, 1); \
    } while(0)


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
    struct expr *pending_dummy_fns;
    struct function *pending_functions;
    struct expr *closure_syms;

    struct scope *outer_scope;
    struct scope *nearest_function_scope;

    struct binding *bindings;
    uint num_bindings;
    uint max_bindings;

    uint depth;
    enum scope_kind kind;
};


static struct expr *expr_create(struct peval_ctx *ctx, uint expr_type, struct expr *antecedent) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    e->antecedent = antecedent;
    return e;
}
static struct expr *unit_create(struct peval_ctx *ctx, struct expr *antecedent) {
    struct expr *e = expr_create(ctx, EXPR_CONST, antecedent);
    e->u._const.type = &type_unit;
    return e;
}

static struct expr *dup_expr(struct peval_ctx *ctx, struct expr *e, struct expr *antecedent) {
    struct expr *e_copy = malloc(sizeof(struct expr));
    *e_copy = *e;
    e_copy->antecedent = antecedent;
    e_copy->source_text.ptr = NULL;
    e_copy->source_text.len = 0;
    return e_copy;
}
static struct expr_decl *dup_decl(struct peval_ctx *ctx, struct expr_decl *f) {
    struct expr_decl *copy = malloc(sizeof(struct expr_decl));
    *copy = *f;
    return copy;
}
static struct expr_link *dup_arg(struct peval_ctx *ctx, struct expr_link *a) {
    struct expr_link *copy = malloc(sizeof(struct expr_link));
    *copy = *a;
    return copy;
}
