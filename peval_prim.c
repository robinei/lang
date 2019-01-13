#include "peval.h"
#include "peval_internal.h"

static int bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CONST);
    if (e->c.type->type != TYPE_BOOL) {
        PEVAL_ERR(e, "expected bool");
    }
    return e->c._bool;
}

static int int_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CONST);
    if (e->c.type->type != TYPE_INT) {
        PEVAL_ERR(e, "expected int");
    }
    return e->c._int;
}

static int const_eq(struct peval_ctx *ctx, struct expr *a, struct expr *b) {
    assert(a->expr == EXPR_CONST && b->expr == EXPR_CONST);
    if (a->c.type != b->c.type) {
        return 0;
    }
    switch (a->c.type->type) {
    case TYPE_TYPE:
        return a->c.typeval == b->c.typeval;
    case TYPE_UNIT:
        return 1;
    case TYPE_BOOL:
        return a->c._bool == b->c._bool;
    case TYPE_INT:
        return a->c._int == b->c._int;
    default:
        PEVAL_ERR(a, "equality not implemented for type");
        return 0;
    }
}

#define ARG(N) (e_new.prim.arg_exprs[N])
#define PEVAL_ARG(N) ARG(N) = peval(ctx, ARG(N))
#define ARG_CONST(N) (ARG(N)->expr == EXPR_CONST)

#define BINOP(op, value_getter) \
    value_getter(ctx, ARG(0)) op value_getter(ctx, ARG(1))


#define HANDLE_UNOP(type_ptr, value_field, value_expression) \
    PEVAL_ARG(0); \
    if (ARG_CONST(0)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST, e); \
        res->c.type = type_ptr; \
        res->c.value_field = value_expression; \
        return res; \
    } \
    break;

#define HANDLE_BINOP(type_ptr, value_field, value_expression) \
    PEVAL_ARG(0); \
    PEVAL_ARG(1); \
    if (ARG_CONST(0) && ARG_CONST(1)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST, e); \
        res->c.type = type_ptr; \
        res->c.value_field = value_expression; \
        return res; \
    } \
    break;



static void splice_visitor(struct expr_visit_ctx *visit_ctx, struct expr *e) {
    if (e->expr == EXPR_PRIM && e->prim.prim == PRIM_SPLICE) {
        struct peval_ctx *ctx = visit_ctx->ctx;
        struct expr *expr = peval(ctx, e->prim.arg_exprs[0]);
        if (expr->expr != EXPR_CONST) {
            PEVAL_ERR(e, "splice argument not computable at compile time");
        }
        if (expr->c.type->type != TYPE_EXPR) {
            PEVAL_ERR(e, "can only splice Expr values");
        }
        *e = *expr->c.expr;
    }
    else {
        expr_visit_children(visit_ctx, e);
    }
}



struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;

    assert(e_new.expr == EXPR_PRIM);

    switch (e_new.prim.prim) {
    case PRIM_PLUS: PEVAL_ARG(0); int_value(ctx, ARG(0)); return ARG(0);
    case PRIM_NEGATE: HANDLE_UNOP(&type_int, _int, -int_value(ctx, ARG(0)))
    case PRIM_LOGI_NOT: HANDLE_UNOP(&type_bool, _bool, !bool_value(ctx, ARG(0)))
    case PRIM_BITWISE_NOT: HANDLE_UNOP(&type_int, _int, ~int_value(ctx, ARG(0)))

    case PRIM_SEQ:
        PEVAL_ARG(0);
        PEVAL_ARG(1);
        if (ARG_CONST(0)) {
            return ARG(1);
        }
        break;
    case PRIM_LOGI_OR:
        PEVAL_ARG(0);
        if (ARG_CONST(0)) {
            return !bool_value(ctx, ARG(0)) ? peval(ctx, ARG(1)) : ARG(0);
        }
        break;
    case PRIM_LOGI_AND:
        PEVAL_ARG(0);
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
        PEVAL_ARG(0);
        if (ARG_CONST(0)) {
            ++ctx->assert_count;
            if (!bool_value(ctx, ARG(0))) {
                ++ctx->assert_fails;
                PEVAL_ERR(e, "assertion failure!");
            }
            return unit_create(ctx, e);
        }
        break;

    case PRIM_QUOTE:
        if (ctx->force_full_expansion) {
            struct expr *res = expr_create(ctx, EXPR_CONST, e);

            struct expr_visit_ctx visit_ctx;
            visit_ctx.visitor = splice_visitor;
            visit_ctx.ctx = ctx;

            res->c.type = &type_expr;
            res->c.expr = expr_visit(&visit_ctx, ARG(0));
            return res;
        }
        break;

    case PRIM_SPLICE:
        ++ctx->force_full_expansion;
        PEVAL_ARG(0);
        --ctx->force_full_expansion;
        if (!ARG_CONST(0)) {
            PEVAL_ERR(e, "'splice' expected compile-time computable argument");
        }
        if (ARG(0)->c.type->type != TYPE_EXPR) {
            PEVAL_ERR(e, "'splice' expected value of type 'Expr'");
        }
        return peval(ctx, ARG(0)->c.expr);

    case PRIM_PRINT:
        PEVAL_ARG(0);
        if (ctx->force_full_expansion) {
            struct print_ctx print_ctx = { 0, };
            print_expr(&print_ctx, ARG(0));
            printf("\n");
            return unit_create(ctx, e);
        }
        break;

    case PRIM_STATIC:
        ++ctx->force_full_expansion;
        PEVAL_ARG(0);
        --ctx->force_full_expansion;
        return ARG(0);

    default:
        PEVAL_ERR(e, "invalid primitive");
    }

    if (ARG(0) != e->prim.arg_exprs[0] || ARG(1) != e->prim.arg_exprs[1]) {
        return dup_expr(ctx, &e_new, e);
    }

    return e;
}
