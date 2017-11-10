#include "peval.h"
#include <assert.h>
#include <stdlib.h>

static struct expr *expr_create(struct peval_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}

static int int_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CONST);
    if (e->u._const.type->type != TYPE_INT) {
        peval_error(ctx, "expected int");
    }
    return e->u._const.u._int;
}

static int const_eq(struct peval_ctx *ctx, struct expr *a, struct expr *b) {
    assert(a->expr == EXPR_CONST && b->expr == EXPR_CONST);
    if (a->u._const.type != b->u._const.type) {
        return 0;
    }
    switch (a->u._const.type->type) {
    case TYPE_TYPE:
        return a->u._const.u.type == b->u._const.u.type;
    case TYPE_BOOL:
        return a->u._const.u._bool == b->u._const.u._bool;
    case TYPE_INT:
        return a->u._const.u._int == b->u._const.u._int;
    default:
        peval_error(ctx, "equality not implemented for type");
        return 0;
    }
}

#define EQUALS() const_eq(ctx, e->u.prim.arg_expr0, e->u.prim.arg_expr1)

#define BINOP(op, value_getter) \
    value_getter(ctx, e->u.prim.arg_expr0) op value_getter(ctx, e->u.prim.arg_expr1)

#define RETURN_CONST(type_ptr, value_field, value_expression) \
    { \
        struct expr *e_new = expr_create(ctx, EXPR_CONST); \
        e_new->u._const.type = type_ptr; \
        e_new->u._const.u.value_field = value_expression; \
        return e_new; \
    }

struct expr *eval_prim(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_PRIM);

    switch (e->u.prim.prim) {
    case PRIM_SEQ: return e->u.prim.arg_expr1;
    case PRIM_EQ: RETURN_CONST(&type_bool, _bool, EQUALS())
    case PRIM_NEQ: RETURN_CONST(&type_bool, _bool, !EQUALS())
    case PRIM_ADD: RETURN_CONST(&type_int, _int, BINOP(+, int_value))
    case PRIM_SUB: RETURN_CONST(&type_int, _int, BINOP(-, int_value))
    case PRIM_MUL: RETURN_CONST(&type_int, _int, BINOP(*, int_value))
    case PRIM_DIV: RETURN_CONST(&type_int, _int, BINOP(/ , int_value))
    default:
        peval_error(ctx, "invalid primitive");
        return NULL;
    }
}
