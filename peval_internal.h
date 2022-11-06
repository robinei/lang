#include "error.h"
#include <stdlib.h>
#include <assert.h>

#define PEVAL_ERR(exp, ...) \
    do { \
        error_emit(ctx->err_ctx, ERROR_CATEGORY_ERROR, (exp)->source_pos, __VA_ARGS__); \
        longjmp(ctx->error_jmp_buf, 1); \
    } while(0)


static struct expr *expr_create(struct peval_ctx *ctx, uint expr_type, struct expr *antecedent) {
    struct expr *e = allocate(ctx->arena, sizeof(struct expr));
    e->kind = expr_type;
    e->source_pos = antecedent ? antecedent->source_pos : 0;
    return e;
}
static struct expr *unit_create(struct peval_ctx *ctx, struct expr *antecedent) {
    struct expr *e = expr_create(ctx, EXPR_CONST, antecedent);
    e->t = &type_unit;
    return e;
}

static struct expr *dup_expr(struct peval_ctx *ctx, struct expr *e, struct expr *antecedent) {
    struct expr *e_copy = allocate(ctx->arena, sizeof(struct expr));
    *e_copy = *e;
    e_copy->source_pos = antecedent ? antecedent->source_pos : 0;
    return e_copy;
}
static struct expr_decl *dup_decl(struct peval_ctx *ctx, struct expr_decl *f) {
    struct expr_decl *copy = allocate(ctx->arena, sizeof(struct expr_decl));
    *copy = *f;
    return copy;
}
static struct expr_link *dup_link(struct peval_ctx *ctx, struct expr_link *a) {
    struct expr_link *copy = allocate(ctx->arena, sizeof(struct expr_link));
    *copy = *a;
    return copy;
}
