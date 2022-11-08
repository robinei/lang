#include "peval.h"
#include "peval_internal.h"

static int bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CONST);
    if (e->t->kind != TYPE_BOOL) {
        PEVAL_ERR(e, "expected bool");
    }
    return e->c.boolean;
}

static void splice_visitor(struct expr_visit_ctx *visit_ctx, struct expr *e) {
    if (e->kind == EXPR_PRIM && e->prim.kind == PRIM_SPLICE) {
        struct peval_ctx *ctx = visit_ctx->ctx;
        struct expr *expr = peval(ctx, e->prim.arg_exprs[0]);
        if (expr->kind != EXPR_CONST) {
            PEVAL_ERR(e, "splice argument not computable at compile time");
        }
        if (expr->t->kind != TYPE_EXPR) {
            PEVAL_ERR(e, "can only splice Expr values");
        }
        *e = *expr->c.expr;
    }
    else {
        expr_visit_children(visit_ctx, e);
    }
}


#define ARG(N) (e_new.prim.arg_exprs[N])
#define PEVAL_ARG(N) ARG(N) = peval(ctx, ARG(N))
#define ARG_CONST(N) (ARG(N)->kind == EXPR_CONST)
#define PEVAL_ARGS() PEVAL_ARG(0); PEVAL_ARG(1)
#define ARGS_CONST() ARG_CONST(0) && ARG_CONST(1)

#define RETURN_CONST(type_ptr, value_field, value_expression) \
    struct expr *res = expr_create(ctx, EXPR_CONST, e); \
    res->t = type_ptr; \
    res->c.value_field = value_expression; \
    return res;


#define BEGIN_UNOP_HANDLER() \
    PEVAL_ARG(0); \
    if (ARG_CONST(0)) {

#define END_UNOP_HANDLER() \
        PEVAL_ERR(e, "bad type"); \
    } \
    break;

#define UNOP(op, field) op ARG(0)->c.field

#define HANDLE_BOOL_UNOP(op) \
    BEGIN_UNOP_HANDLER() \
        if(ARG(0)->t == &type_bool) { RETURN_CONST(&type_bool, boolean, UNOP(op, boolean)) } \
    END_UNOP_HANDLER()
    
#define HANDLE_INT_UINT_UNOP(op) \
    BEGIN_UNOP_HANDLER() \
        if(ARG(0)->t == &type_int) { RETURN_CONST(&type_int, integer, UNOP(op, integer)) } \
        if(ARG(0)->t == &type_uint) { RETURN_CONST(&type_uint, uinteger, UNOP(op, uinteger)) } \
    END_UNOP_HANDLER()

#define HANDLE_INT_UINT_REAL_UNOP(op) \
    BEGIN_UNOP_HANDLER() \
        if(ARG(0)->t == &type_int) { RETURN_CONST(&type_int, integer, UNOP(op, integer)) } \
        if(ARG(0)->t == &type_uint) { RETURN_CONST(&type_uint, uinteger, UNOP(op, uinteger)) } \
        if(ARG(0)->t == &type_real) { RETURN_CONST(&type_real, real, UNOP(op, real)) } \
    END_UNOP_HANDLER()


#define BEGIN_BINOP_HANDLER() \
    PEVAL_ARGS(); \
    if (ARGS_CONST()) { \
        if (ARG(0)->t != ARG(1)->t) { \
            PEVAL_ERR(e, "type mismatch"); \
        }

#define END_BINOP_HANDLER() \
        PEVAL_ERR(e, "bad types"); \
    } \
    break;

#define BINOP(op, field) ARG(0)->c.field op ARG(1)->c.field

#define HANDLE_INT_UINT_REAL_BINOP(op) \
    BEGIN_BINOP_HANDLER() \
        if (ARG(0)->t == &type_int) { RETURN_CONST(&type_int, integer, BINOP(op, integer)); } \
        if (ARG(0)->t == &type_uint) { RETURN_CONST(&type_uint, uinteger, BINOP(op, uinteger)); } \
        if (ARG(0)->t == &type_real) { RETURN_CONST(&type_real, real, BINOP(op, real)); } \
    END_BINOP_HANDLER()

#define HANDLE_INT_UINT_BINOP(op) \
    BEGIN_BINOP_HANDLER() \
        if (ARG(0)->t == &type_int) { RETURN_CONST(&type_int, integer, BINOP(op, integer)); } \
        if (ARG(0)->t == &type_uint) { RETURN_CONST(&type_uint, uinteger, BINOP(op, uinteger)); } \
    END_BINOP_HANDLER()

#define HANDLE_CMP_BINOP(op) \
    BEGIN_BINOP_HANDLER() \
        if (ARG(0)->t == &type_int) { RETURN_CONST(&type_bool, boolean, BINOP(op, integer)); } \
        if (ARG(0)->t == &type_uint) { RETURN_CONST(&type_bool, boolean, BINOP(op, uinteger)); } \
        if (ARG(0)->t == &type_real) { RETURN_CONST(&type_bool, boolean, BINOP(op, real)); } \
    END_BINOP_HANDLER()

#define HANDLE_SHIFT_BINOP(op) \
    PEVAL_ARGS(); \
    if (ARGS_CONST()) { \
        if (ARG(1)->t == &type_int) { \
            if (ARG(1)->c.integer < 0) { PEVAL_ERR(ARG(1), "can't shift by negative number"); } \
        } else if (ARG(1)->t != &type_uint) { \
            PEVAL_ERR(ARG(1), "can't shift by non-integer"); \
        } \
        if (ARG(0)->t == &type_int) { \
            if (ARG(0)->c.integer < 0) { PEVAL_ERR(ARG(0), "can't shift negative number"); } \
            RETURN_CONST(&type_int, integer, BINOP(op, integer)); \
        } else if (ARG(0)->t == &type_uint) { \
            RETURN_CONST(&type_uint, uinteger, BINOP(op, uinteger)); \
        } else { \
            PEVAL_ERR(ARG(0), "can't shift non-integer"); \
        } \
    } \
    break;


struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;

    assert(e_new.kind == EXPR_PRIM);

    switch (e_new.prim.kind) {
    case PRIM_PLUS: HANDLE_INT_UINT_REAL_UNOP(+)
    case PRIM_NEGATE: HANDLE_INT_UINT_REAL_UNOP(-)
    case PRIM_LOGI_NOT: HANDLE_BOOL_UNOP(!)
    case PRIM_BITWISE_NOT: HANDLE_INT_UINT_UNOP(~)

    case PRIM_SEQ:
        PEVAL_ARGS();
        if (ARG_CONST(0)) {
            return ARG(1);
        }
        break;

    case PRIM_ASSIGN:
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
            if (target->kind != EXPR_CONST || target->t != &type_type || target->c.type->kind != TYPE_STRUCT) {
                PEVAL_ERR(field, "invalid assignment target");
            }
            struct type *type = target->c.type;
            type_set_attr(type, field->sym, ARG(1));
            return unit_create(ctx, e);
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

    case PRIM_BITWISE_OR:  HANDLE_INT_UINT_BINOP(|)
    case PRIM_BITWISE_XOR: HANDLE_INT_UINT_BINOP(^)
    case PRIM_BITWISE_AND: HANDLE_INT_UINT_BINOP(&)

    case PRIM_EQ:  PEVAL_ARGS(); if (ARGS_CONST()) { RETURN_CONST(&type_bool, boolean,  const_eq(ctx, ARG(0), ARG(1))) }
    case PRIM_NEQ: PEVAL_ARGS(); if (ARGS_CONST()) { RETURN_CONST(&type_bool, boolean, !const_eq(ctx, ARG(0), ARG(1))) }

    case PRIM_LT:   HANDLE_CMP_BINOP(<)
    case PRIM_GT:   HANDLE_CMP_BINOP(>)
    case PRIM_LTEQ: HANDLE_CMP_BINOP(<=)
    case PRIM_GTEQ: HANDLE_CMP_BINOP(>=)

    case PRIM_BITWISE_LSH: HANDLE_SHIFT_BINOP(<<)
    case PRIM_BITWISE_RSH: HANDLE_SHIFT_BINOP(>>)

    case PRIM_ADD: HANDLE_INT_UINT_REAL_BINOP(+)
    case PRIM_SUB: HANDLE_INT_UINT_REAL_BINOP(-)
    case PRIM_MUL: HANDLE_INT_UINT_REAL_BINOP(*)
    case PRIM_DIV: HANDLE_INT_UINT_REAL_BINOP(/)
    case PRIM_MOD: HANDLE_INT_UINT_BINOP(%)
    
    case PRIM_DOT:
        if (ARG(1)->kind != EXPR_SYM) {
            PEVAL_ERR(ARG(1), "expected symbol");
        }
        PEVAL_ARG(0);
        if (ARG_CONST(0)) {
            if (ARG(0)->t != &type_type) {
                PEVAL_ERR(ARG(0), "expected type");
            }
            struct expr *sym = ARG(1);
            struct type *type = ARG(0)->c.type;
            struct expr *res = type_get_attr(type, sym->sym);
            if (!res) {
                PEVAL_ERR(sym, "non-existing attribute: %s", sym->sym->data);
            }
            return dup_expr(ctx, res, e);
        }
        break;

    case PRIM_ASSERT:
        PEVAL_ARG(0);
        if (ARG_CONST(0)) {
            ++ctx->mod_ctx->asserts_hit;
            if (!bool_value(ctx, ARG(0))) {
                ++ctx->mod_ctx->asserts_failed;
                PEVAL_ERR(e, "assertion failure!");
            }
            return unit_create(ctx, e);
        }
        break;

    case PRIM_QUOTE:
        if (ctx->force_full_expansion) {
            struct expr *res = expr_create(ctx, EXPR_CONST, e);
            res->t = &type_expr;
            res->c.expr = expr_run_visitor(ARG(0), splice_visitor, ctx, ctx->arena);
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
        if (ARG(0)->t->kind != TYPE_EXPR) {
            PEVAL_ERR(e, "'splice' expected argument of type 'Expr'");
        }
        return peval(ctx, ARG(0)->c.expr);

    case PRIM_PRINT:
        PEVAL_ARG(0);
        if (ctx->force_full_expansion) {
            pretty_print(ARG(0));
            return unit_create(ctx, e);
        }
        break;
    
    case PRIM_IMPORT:
        if (ctx->force_full_expansion) {
            PEVAL_ARG(0);
            if (!ARG_CONST(0)) {
                PEVAL_ERR(e, "'import' expected compile-time computable argument");
            }
            if (ARG(0)->t->kind != TYPE_STRING) {
                PEVAL_ERR(e, "'import' expected argument of type 'String'");
            }
            slice_t path = ARG(0)->c.string;
            struct module_ctx *mod_ctx = module_load(ctx->global_ctx, ctx->mod_ctx, path);
            if (!mod_ctx) {
                PEVAL_ERR(e, "error importing module '%.*s'", path.len, path.ptr);
            }
            // TODO: modules should be in a global registry
            struct expr *res = expr_create(ctx, EXPR_CONST, e);
            res->t = &type_type;
            res->c.type = mod_ctx->module_type;
            return res;
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
