#include "peval.h"
#include "peval_internal.h"

static int bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr_kind == EXPR_CONST);
    if (e->c.tag->type_kind != TYPE_BOOL) {
        PEVAL_ERR(e, "expected bool");
    }
    return e->c.boolean;
}

static int int_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr_kind == EXPR_CONST);
    if (e->c.tag->type_kind != TYPE_INT) {
        PEVAL_ERR(e, "expected int");
    }
    return e->c.integer;
}

static int const_eq(struct peval_ctx *ctx, struct expr *a, struct expr *b) {
    assert(a->expr_kind == EXPR_CONST && b->expr_kind == EXPR_CONST);
    if (a->c.tag != b->c.tag) {
        return 0;
    }
    switch (a->c.tag->type_kind) {
    case TYPE_TYPE:
        return a->c.type == b->c.type;
    case TYPE_UNIT:
        return 1;
    case TYPE_BOOL:
        return a->c.boolean == b->c.boolean;
    case TYPE_INT:
        return a->c.integer == b->c.integer;
    default:
        PEVAL_ERR(a, "equality not implemented for type");
        return 0;
    }
}

#define ARG(N) (e_new.prim.arg_exprs[N])
#define PEVAL_ARG(N) ARG(N) = peval(ctx, ARG(N))
#define ARG_CONST(N) (ARG(N)->expr_kind == EXPR_CONST)

#define BINOP(op, value_getter) \
    value_getter(ctx, ARG(0)) op value_getter(ctx, ARG(1))


#define HANDLE_UNOP(type_ptr, value_field, value_expression) \
    PEVAL_ARG(0); \
    if (ARG_CONST(0)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST, e); \
        res->c.tag = type_ptr; \
        res->c.value_field = value_expression; \
        return res; \
    } \
    break;

#define HANDLE_BINOP(type_ptr, value_field, value_expression) \
    PEVAL_ARG(0); \
    PEVAL_ARG(1); \
    if (ARG_CONST(0) && ARG_CONST(1)) { \
        struct expr *res = expr_create(ctx, EXPR_CONST, e); \
        res->c.tag = type_ptr; \
        res->c.value_field = value_expression; \
        return res; \
    } \
    break;



static void splice_visitor(struct expr_visit_ctx *visit_ctx, struct expr *e) {
    if (e->expr_kind == EXPR_PRIM && e->prim.prim == PRIM_SPLICE) {
        struct peval_ctx *ctx = visit_ctx->ctx;
        struct expr *expr = peval(ctx, e->prim.arg_exprs[0]);
        if (expr->expr_kind != EXPR_CONST) {
            PEVAL_ERR(e, "splice argument not computable at compile time");
        }
        if (expr->c.tag->type_kind != TYPE_EXPR) {
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

    assert(e_new.expr_kind == EXPR_PRIM);

    switch (e_new.prim.prim) {
    case PRIM_PLUS: PEVAL_ARG(0); int_value(ctx, ARG(0)); return ARG(0);
    case PRIM_NEGATE: HANDLE_UNOP(&type_int, integer, -int_value(ctx, ARG(0)))
    case PRIM_LOGI_NOT: HANDLE_UNOP(&type_bool, boolean, !bool_value(ctx, ARG(0)))
    case PRIM_BITWISE_NOT: HANDLE_UNOP(&type_int, integer, ~int_value(ctx, ARG(0)))

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

    case PRIM_BITWISE_OR: HANDLE_BINOP(&type_int, integer, BINOP(|, int_value))
    case PRIM_BITWISE_XOR: HANDLE_BINOP(&type_int, integer, BINOP(^ , int_value))
    case PRIM_BITWISE_AND: HANDLE_BINOP(&type_int, integer, BINOP(& , int_value))

    case PRIM_EQ: HANDLE_BINOP(&type_bool, boolean, const_eq(ctx, ARG(0), ARG(1)))
    case PRIM_NEQ: HANDLE_BINOP(&type_bool, boolean, !const_eq(ctx, ARG(0), ARG(1)))

    case PRIM_LT: HANDLE_BINOP(&type_bool, boolean, BINOP(<, int_value))
    case PRIM_GT: HANDLE_BINOP(&type_bool, boolean, BINOP(>, int_value))
    case PRIM_LTEQ: HANDLE_BINOP(&type_bool, boolean, BINOP(<=, int_value))
    case PRIM_GTEQ: HANDLE_BINOP(&type_bool, boolean, BINOP(>=, int_value))

    case PRIM_BITWISE_LSH: HANDLE_BINOP(&type_int, integer, BINOP(<<, int_value))
    case PRIM_BITWISE_RSH: HANDLE_BINOP(&type_int, integer, BINOP(>>, int_value))

    case PRIM_ADD: HANDLE_BINOP(&type_int, integer, BINOP(+, int_value))
    case PRIM_SUB: HANDLE_BINOP(&type_int, integer, BINOP(-, int_value))
    case PRIM_MUL: HANDLE_BINOP(&type_int, integer, BINOP(*, int_value))
    case PRIM_DIV: HANDLE_BINOP(&type_int, integer, BINOP(/ , int_value))
    case PRIM_MOD: HANDLE_BINOP(&type_int, integer, BINOP(% , int_value))

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

            res->c.tag = &type_expr;
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
        if (ARG(0)->c.tag->type_kind != TYPE_EXPR) {
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
