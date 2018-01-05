#include "peval.h"
#include <assert.h>
#include <stdlib.h>

static struct expr *expr_create(struct peval_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}
static struct expr *dup_expr(struct peval_ctx *ctx, struct expr *e) {
    struct expr *e_copy = calloc(1, sizeof(struct expr));
    *e_copy = *e;
    return e_copy;
}

static int bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CONST);
    if (e->u._const.type->type != TYPE_BOOL) {
        peval_error(ctx, "expected bool");
    }
    return e->u._const.u._bool;
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

#define ARG(N) (e_new.u.prim.arg_exprs[N])
#define PEVAL_ARG(N) ARG(N) = peval(ctx, ARG(N))
#define ARG_CONST(N) (ARG(N)->expr == EXPR_CONST)

#define BINOP(op, value_getter) \
    value_getter(ctx, ARG(0)) op value_getter(ctx, ARG(1))


#define HANDLE_UNOP(type_ptr, value_field, value_expression) \
    if (ARG_CONST(0)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST); \
        res->u._const.type = type_ptr; \
        res->u._const.u.value_field = value_expression; \
        return res; \
    } \
    break;

#define HANDLE_BINOP(type_ptr, value_field, value_expression) \
    PEVAL_ARG(1); \
    if (ARG_CONST(0) && ARG_CONST(1)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST); \
        res->u._const.type = type_ptr; \
        res->u._const.u.value_field = value_expression; \
        return res; \
    } \
    break;

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;

    assert(e_new.expr == EXPR_PRIM);

    PEVAL_ARG(0);

    switch (e_new.u.prim.prim) {
    case PRIM_PLUS: return ARG(0);
    case PRIM_NEGATE: HANDLE_UNOP(&type_int, _int, -int_value(ctx, ARG(0)))
    case PRIM_LOGI_NOT: HANDLE_UNOP(&type_bool, _bool, !bool_value(ctx, ARG(0)))
    case PRIM_BITWISE_NOT: HANDLE_UNOP(&type_int, _int, ~int_value(ctx, ARG(0)))

    case PRIM_SEQ:
        PEVAL_ARG(1);
        if (ARG_CONST(0)) {
            return ARG(1);
        }
        break;
    case PRIM_LOGI_OR:
        if (ARG_CONST(0)) {
            return !bool_value(ctx, ARG(0)) ? peval(ctx, ARG(1)) : ARG(0);
        }
        break;
    case PRIM_LOGI_AND:
        if (ARG_CONST(0)) {
            return bool_value(ctx, ARG(0)) ? peval(ctx, ARG(1)) : ARG(0);
        }
        break;

    case PRIM_BITWISE_OR: HANDLE_BINOP(&type_int, _int, BINOP(|, int_value))
    case PRIM_BITWISE_XOR: HANDLE_BINOP(&type_int, _int, BINOP(^ , int_value))
    case PRIM_BITWISE_AND: HANDLE_BINOP(&type_int, _int, BINOP(& , int_value))

    case PRIM_EQ: HANDLE_BINOP(&type_bool, _bool, const_eq(ctx, ARG(0), ARG(1)))
    case PRIM_NEQ: HANDLE_BINOP(&type_bool, _bool, !const_eq(ctx, ARG(0), ARG(1)))

    case PRIM_LT: HANDLE_BINOP(&type_bool, _bool, BINOP(<, int_value))
    case PRIM_GT: HANDLE_BINOP(&type_bool, _bool, BINOP(>, int_value))
    case PRIM_LTEQ: HANDLE_BINOP(&type_bool, _bool, BINOP(<=, int_value))
    case PRIM_GTEQ: HANDLE_BINOP(&type_bool, _bool, BINOP(>=, int_value))

    case PRIM_BITWISE_LSH: HANDLE_BINOP(&type_int, _int, BINOP(<<, int_value))
    case PRIM_BITWISE_RSH: HANDLE_BINOP(&type_int, _int, BINOP(>>, int_value))

    case PRIM_ADD: HANDLE_BINOP(&type_int, _int, BINOP(+, int_value))
    case PRIM_SUB: HANDLE_BINOP(&type_int, _int, BINOP(-, int_value))
    case PRIM_MUL: HANDLE_BINOP(&type_int, _int, BINOP(*, int_value))
    case PRIM_DIV: HANDLE_BINOP(&type_int, _int, BINOP(/ , int_value))
    case PRIM_MOD: HANDLE_BINOP(&type_int, _int, BINOP(% , int_value))

    case PRIM_ASSERT:
        if (ARG_CONST(0) && ctx->allow_side_effects) {
            ++ctx->assert_count;
            if (!bool_value(ctx, ARG(0))) {
                peval_error(ctx, "assertion failure!");
            }
            return &expr_unit;
        }
        break;

    default:
        peval_error(ctx, "invalid primitive");
        return NULL;
    }

    if (ARG(0) != e->u.prim.arg_exprs[0] || ARG(1) != e->u.prim.arg_exprs[1]) {
        return dup_expr(ctx, &e_new);
    }

    return e;
}
