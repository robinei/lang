#include "error.h"
#include <stdlib.h>
#include <assert.h>

#define PEVAL_ERR(exp, ...) \
    do { \
        error_emit(ctx->err_ctx, ERROR_CATEGORY_ERROR, (exp)->source_pos, __VA_ARGS__); \
        longjmp(ctx->error_jmp_buf, 1); \
    } while(0)


static struct expr *expr_create(struct peval_ctx *ctx, enum expr_kind kind, struct expr *antecedent) {
    struct expr *e = allocate(ctx->arena, sizeof(struct expr));
    e->kind = kind;
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

static bool const_eq(struct expr *a, struct expr *b) {
    assert(a->kind == EXPR_CONST && b->kind == EXPR_CONST);
    if (a->t != b->t) {
        return false;
    }
    switch (a->t->kind) {
    case TYPE_TYPE:
        return a->c.type == b->c.type;
    case TYPE_UNIT:
        return true;
    case TYPE_BOOL:
        return a->c.boolean == b->c.boolean;
    case TYPE_INT:
        return a->c.integer == b->c.integer;
    case TYPE_UINT:
        return a->c.uinteger == b->c.uinteger;
    case TYPE_REAL:
        return a->c.real == b->c.real;
    case TYPE_STRING:
        return slice_equals(a->c.string, b->c.string);
    default:
        assert(0 && "equality not implemented for type");
        return false;
    }
}

