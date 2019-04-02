#include "peval.h"
#include "peval_internal.h"

static int bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CONST);
    if (e->c.tag->kind != TYPE_BOOL) {
        PEVAL_ERR(e, "expected bool");
    }
    return e->c.boolean;
}

static int int_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CONST);
    if (e->c.tag->kind != TYPE_INT) {
        PEVAL_ERR(e, "expected int");
    }
    return e->c.integer;
}

static int const_eq(struct peval_ctx *ctx, struct expr *a, struct expr *b) {
    assert(a->kind == EXPR_CONST && b->kind == EXPR_CONST);
    if (a->c.tag != b->c.tag) {
        return 0;
    }
    switch (a->c.tag->kind) {
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
#define ARG_CONST(N) (ARG(N)->kind == EXPR_CONST)

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
    if (e->kind == EXPR_PRIM && e->prim.kind == PRIM_SPLICE) {
        struct peval_ctx *ctx = visit_ctx->ctx;
        struct expr *expr = peval(ctx, e->prim.arg_exprs[0]);
        if (expr->kind != EXPR_CONST) {
            PEVAL_ERR(e, "splice argument not computable at compile time");
        }
        if (expr->c.tag->kind != TYPE_EXPR) {
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

    assert(e_new.kind == EXPR_PRIM);

    switch (e_new.prim.kind) {
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

    case PRIM_ASSIGN: {
        PEVAL_ARG(1);
        if (ARG_CONST(1)) {
            if (ARG(0)->kind != EXPR_PRIM || ARG(0)->prim.kind != PRIM_DOT) {
                PEVAL_ERR(ARG(0), "invalid assignment target");
            }
            struct expr *field = ARG(0)->prim.arg_exprs[1];
            if (field->kind != EXPR_SYM) {
                PEVAL_ERR(field, "expected symbol");
            }
            struct expr *target = peval(ctx, ARG(0)->prim.arg_exprs[0]);
            if (target->kind != EXPR_CONST || target->c.tag != &type_type || target->c.type->kind != TYPE_STRUCT) {
                PEVAL_ERR(field, "invalid assignment target");
            }
            struct type *type = target->c.type;
            struct type_attr **last = &type->attrs;
            while (*last) {
                last = &((*last)->next);
            }
            *last = calloc(1, sizeof(struct type_attr));
            (*last)->name = field->sym;
            (*last)->value_expr = ARG(1);
            return unit_create(ctx, e);
        }
        break;
    }

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

    case PRIM_DOT: {
        if (ARG(1)->kind != EXPR_SYM) {
            PEVAL_ERR(ARG(1), "expected symbol");
        }
        PEVAL_ARG(0);
        if (ARG_CONST(0)) {
            if (ARG(0)->c.tag != &type_type) {
                PEVAL_ERR(ARG(0), "expected type");
            }
            struct expr *sym = ARG(1);
            struct type *type = ARG(0)->c.type;
            for (struct type_attr *a = type->attrs; a; a = a->next) {
                if (a->name == sym->sym) {
                    struct expr *res = expr_create(ctx, EXPR_CONST, e);
                    res->c = a->value_expr->c;
                    return res;
                }
            }

            PEVAL_ERR(sym, "non-existing attribute: %s", sym->sym->data);
        }
        break;
    }

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
        if (ARG(0)->c.tag->kind != TYPE_EXPR) {
            PEVAL_ERR(e, "'splice' expected value of type 'Expr'");
        }
        return peval(ctx, ARG(0)->c.expr);

    case PRIM_PRINT:
        PEVAL_ARG(0);
        if (ctx->force_full_expansion) {
            pretty_print(ARG(0));
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
