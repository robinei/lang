#include "peval.h"
#include "error.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>
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

static bool bool_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CONST);
    if (e->t->kind != TYPE_BOOL) {
        PEVAL_ERR(e, "expected bool");
    }
    return e->c.boolean;
}



static struct expr *peval_type(struct peval_ctx *ctx, struct expr *e) {
    if (!e) {
        return e;
    }
    
    ++ctx->force_full_expansion;
    struct expr *result = peval(ctx, e);
    --ctx->force_full_expansion;

    if (result->kind != EXPR_CONST) {
        PEVAL_ERR(result, "expected constant type expression");
    }
    if (result->t->kind != TYPE_TYPE) {
        PEVAL_ERR(result, "expected type expression");
    }
    return result;
}

static void check_type(struct peval_ctx *ctx, struct expr *e, struct expr *e_type) {
    if (!e || !e_type || e->kind != EXPR_CONST || e_type->kind != EXPR_CONST) {
        return;
    }
    assert(e_type->t->kind == TYPE_TYPE);
    if (e_type->c.type != e->t) {
        PEVAL_ERR(e, "type mismatch. expected %s. found %s",
            type_names[e_type->c.type->kind],
            type_names[e->t->kind]);
    }
}

#define FNV1A(location, seed) fnv1a((unsigned char *)&(location), sizeof(location), seed)

struct arg_array {
    uint arg_count;
    struct expr *args[0];
};

static uint arg_array_hash(struct arg_array *a) {
    uint hash = FNV_SEED;
    for (uint i = 0; i < a->arg_count; ++i) {
        struct expr *arg = a->args[i];
        assert(arg->kind == EXPR_CONST);
        hash = FNV1A(arg->t->kind, hash);
        switch (arg->t->kind) {
            case TYPE_TYPE: hash = FNV1A(arg->c.type, hash); break; /* hash pointer */
            case TYPE_BOOL: hash = FNV1A(arg->c.boolean, hash); break;
            case TYPE_INT: hash = FNV1A(arg->c.integer, hash); break;
            case TYPE_UINT: hash = FNV1A(arg->c.uinteger, hash); break;
            case TYPE_REAL: hash = FNV1A(arg->c.real, hash); break;
            case TYPE_EXPR: hash = FNV1A(arg->c.expr, hash); break; /* hash pointer */
            default: assert(0 && "hashing unimplemented for type"); break;
        }
    }
    return hash;
}

static bool arg_array_eq(struct arg_array *a, struct arg_array *b) {
    if (a->arg_count != b->arg_count) {
        return false;
    }
    for (uint i = 0; i < a->arg_count; ++i) {
        if (!const_eq(a->args[i], b->args[i])) {
            return false;
        }
    }
    return true;
}

static struct function *create_function(struct peval_ctx *ctx) {
    struct function *func = allocate(ctx->arena, sizeof(struct function));
    hashtable_init(SPECIALIZATIONS_TABLE, func->specializations, ctx->arena, 0);
    return func;
}

static struct expr *create_new_call(struct peval_ctx *ctx, struct expr *orig_call, struct expr *callable, struct expr **args, uint arg_count) {
    bool changed = callable != orig_call->call.callable_expr || arg_count != orig_call->call.arg_count;
    for (int i = 0; !changed && i < arg_count; ++i) {
        changed = args[i] != orig_call->call.args[i];
    }
    if (!changed) {
        return orig_call;
    }
    struct expr temp = *orig_call;
    temp.call.callable_expr = callable;
    temp.call.args = dup_memory(ctx->arena, args, sizeof(*args) * arg_count);
    return dup_memory(ctx->arena, &temp, sizeof(temp));
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CALL);

    struct expr *callable = peval(ctx, e->call.callable_expr);
    uint arg_count = e->call.arg_count;
    struct expr *args[MAX_PARAM];
    for (uint i = 0; i < arg_count; ++i) {
        args[i] = peval(ctx, e->call.args[i]);
    }

    if (callable->kind != EXPR_CONST) {
        return create_new_call(ctx, e, callable, args, arg_count);
    }
    if (callable->t->kind != TYPE_FUN) {
        PEVAL_ERR(callable, "expected a function");
    }

    struct function *func = callable->c.fun;
    struct expr *fun_expr = func->fun_expr;
    if (arg_count != fun_expr->fun_param_count) {
        PEVAL_ERR(e, "function expected %u arguments; %d were provided", fun_expr->fun_param_count, arg_count);
    }
    
    struct scope *prev_scope = ctx->scope;
    assert(prev_scope);
    struct scope *scope = ctx->scope = scope_create(ctx->arena, func->env);

    union {
        struct arg_array specialized_args;
        char _reserve[sizeof(struct arg_array) + sizeof(struct expr *) * MAX_PARAM];
    } u;
    struct fun_param params[MAX_PARAM];
    bool params_changed = false;
    uint remaining_arg_count = 0;
    u.specialized_args.arg_count = 0;
    for (uint i = 0; i < arg_count; ++i) {
        struct expr *a = args[i];
        struct fun_param *p = fun_expr->fun.params + i;

        struct expr *type_expr = peval(ctx, p->type_expr);
        check_type(ctx, a, type_expr);
        params_changed = params_changed || type_expr != p->type_expr;

        if (p->is_static || ctx->force_full_expansion) {
            if (a->kind != EXPR_CONST) {
                PEVAL_ERR(a, "expected static argument");
            }
            params_changed = true;
            u.specialized_args.args[u.specialized_args.arg_count++] = args[i];
        } else {
            args[remaining_arg_count] = args[i];
            params[remaining_arg_count] = *p;
            params[remaining_arg_count].type_expr = type_expr;
            ++remaining_arg_count;
        }

        assert(p->name_expr->kind == EXPR_SYM);
        scope_define(ctx->scope, p->name_expr->sym, a);
    }

    if (ctx->force_full_expansion) {
        assert(remaining_arg_count == 0);
        struct expr *body_expr = peval(ctx, fun_expr->fun.body_expr);
        struct expr *return_type_expr = peval(ctx, fun_expr->fun.return_type_expr);
        check_type(ctx, body_expr, return_type_expr);
        assert(ctx->scope == scope);
        ctx->scope = prev_scope;
        return body_expr;
    }

    if (remaining_arg_count == arg_count) {
        assert(ctx->scope == scope);
        ctx->scope = prev_scope;
        return create_new_call(ctx, e, callable, args, arg_count);
    }

    struct function *new_func;
    bool found;
    hashtable_get(SPECIALIZATIONS_TABLE, func->specializations, &u.specialized_args, new_func, found);
    if (!found) {
        struct expr *body_expr = peval(ctx, fun_expr->fun.body_expr);
        struct expr *return_type_expr = peval(ctx, fun_expr->fun.return_type_expr);
        check_type(ctx, body_expr, return_type_expr);

        new_func = create_function(ctx);
        new_func->name = func->name;
        new_func->env = func->env;
        new_func->fun_expr = expr_create(ctx, EXPR_FUN, fun_expr);
        new_func->fun_expr->fun.params = dup_memory(ctx->arena, params, sizeof(struct fun_param) * remaining_arg_count);
        new_func->fun_expr->fun.return_type_expr = return_type_expr;
        new_func->fun_expr->fun.body_expr = body_expr;
        
        struct arg_array *args_key = dup_memory(ctx->arena, &u.specialized_args,
            sizeof(struct arg_array) + sizeof(struct expr *) * u.specialized_args.arg_count);
        hashtable_put(SPECIALIZATIONS_TABLE, func->specializations, args_key, new_func);
    }

    struct expr *specialized_callable = expr_create(ctx, EXPR_CONST, callable);
    specialized_callable->t = &type_fun;
    specialized_callable->c.fun = new_func;

    assert(ctx->scope == scope);
    ctx->scope = prev_scope;
    return create_new_call(ctx, e, specialized_callable, args, remaining_arg_count);
}

static struct expr *peval_sym(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_SYM);
    struct expr *val = scope_lookup(ctx->scope, e->sym);
    if (!val) {
        PEVAL_ERR(e, "symbol not in scope");
    }
    if (val->kind != EXPR_CONST) {
        return e;
    }
    return dup_expr(ctx, val, e);
}

static struct expr *peval_struct(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_STRUCT);
    struct expr *result = e;

    struct scope *prev_scope = ctx->scope;
    assert(prev_scope);
    struct scope *scope = ctx->scope = scope_create(ctx->arena, ctx->scope);

    struct type *self = allocate(ctx->arena, sizeof(struct type));
    hashtable_init(TYPEATTR_HASHTABLE, self->attrs, &ctx->mod_ctx->alloc.a, 0);
    self->kind = TYPE_STRUCT;
    ctx->scope->self = self;

    struct expr *body = peval(ctx, e->struc.body_expr);
    
    if (body->kind == EXPR_CONST) {
        result = expr_create(ctx, EXPR_CONST, e);
        result->t = &type_type;
        result->c.type = self;
    }

    assert(ctx->scope == scope);
    assert(prev_scope == ctx->scope->parent);
    ctx->scope = prev_scope;

    return result;
}

static struct expr *peval_self(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_SELF);
    assert(ctx->scope);
    struct type *self = NULL;
    for (struct scope *scope = ctx->scope; scope; scope = scope->parent) {
        if (scope->self) {
            self = scope->self;
            break;
        }
    }
    assert(self);
    struct expr *result = expr_create(ctx, EXPR_CONST, e);
    result->t = &type_type;
    result->c.type = self;
    return result;
}

static struct expr *peval_def(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_DEF);
    assert(e->def.name_expr);
    assert(e->def.name_expr->kind == EXPR_SYM);
    assert(e->def.value_expr);

    struct expr e_new = *e;
    e_new.def.type_expr = peval(ctx, e->def.type_expr);
    e_new.def.value_expr = peval(ctx, e->def.value_expr);

    bool changed;
    if (e_new.def.value_expr->kind == EXPR_CONST) {
        scope_define(ctx->scope, e_new.def.name_expr->sym, e_new.def.value_expr);
        memset(&e_new, 0, sizeof(struct expr));
        e_new.kind = EXPR_CONST;
        e_new.t = &type_unit;
        e_new.source_pos = e->source_pos;
        changed = true;
    } else {
        changed = e_new.def.type_expr != e->def.type_expr || e_new.def.value_expr != e->def.value_expr;
    }

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_block(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_BLOCK);

    struct scope *prev_scope = ctx->scope;
    assert(prev_scope);
    struct scope *scope = ctx->scope = scope_create(ctx->arena, ctx->scope);

    bool changed = false;
    struct expr *exprs[MAX_BLOCK];
    uint expr_count = 0;
    for (uint i = 0; i < e->block.expr_count; ++i) {
        exprs[expr_count] = peval(ctx, e->block.exprs[i]);
        if (exprs[expr_count]->kind != EXPR_CONST || i + 1 == e->block.expr_count) {
            changed = changed || exprs[expr_count] != e->block.exprs[i];
            ++expr_count;
        } else {
            changed = true;
        }
    }

    assert(ctx->scope == scope);
    assert(prev_scope == ctx->scope->parent);
    ctx->scope = prev_scope;

    if (expr_count == 1 && exprs[0]->kind != EXPR_DEF) {
        return exprs[0];
    }
    if (changed) {
        struct expr e_new = *e;
        e_new.block.exprs = dup_memory(ctx->arena, exprs, sizeof(struct expr *) * expr_count);
        e_new.block.expr_count = expr_count;
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_fun(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_FUN);
    struct expr *result;

    if (!e->fun.body_expr) {
        /* this is a function type expression */
        peval_type(ctx, e->fun.return_type_expr);
        result = expr_create(ctx, EXPR_CONST, e);
        result->t = &type_type;
        result->c.type = &type_fun;
    } else {
        struct function *func = create_function(ctx);
        func->name = ctx->sym_lambda;
        func->fun_expr = e;
        func->env = ctx->scope;

        result = expr_create(ctx, EXPR_CONST, e);
        result->t = &type_fun;
        result->c.fun = func;
    }

    return result;
}

static struct expr *peval_if(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_COND);
    struct expr e_new = *e;

    struct expr *pred_expr = e_new.cond.pred_expr = peval(ctx, e->cond.pred_expr);

    if (pred_expr->kind == EXPR_CONST) {
        if (pred_expr->t->kind != TYPE_BOOL) {
            PEVAL_ERR(pred_expr, "if conditional must be boolean");
        }
        e_new = *peval(ctx, pred_expr->c.boolean ? e->cond.then_expr : e->cond.else_expr);
        return dup_expr(ctx, &e_new, e);
    }

    e_new.cond.then_expr = peval(ctx, e->cond.then_expr);
    e_new.cond.else_expr = peval(ctx, e->cond.else_expr);

    if (e_new.cond.then_expr->kind == EXPR_CONST && e_new.cond.else_expr->kind == EXPR_CONST &&
        (e_new.cond.then_expr->t != e_new.cond.else_expr->t)) {
        PEVAL_ERR(&e_new, "mismatching types in if arms");
    }

    if (e_new.cond.pred_expr != e->cond.pred_expr ||
        e_new.cond.then_expr != e->cond.then_expr ||
        e_new.cond.else_expr != e->cond.else_expr) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}







#define ARG(N) (e_new.prim.arg_exprs[N])
#define PEVAL_ARG(N) ARG(N) = peval(ctx, ARG(N))
#define ARG_CONST(N) (ARG(N)->kind == EXPR_CONST)
#define ARGS_CONST() ARG_CONST(0) && ARG_CONST(1)

#define RETURN_CONST(type_ptr, value_field, value_expression) \
    struct expr *res = expr_create(ctx, EXPR_CONST, e); \
    res->t = type_ptr; \
    res->c.value_field = value_expression; \
    return res;

#define RETURN_MODIFIED() return ARG(0) != e->prim.arg_exprs[0] || ARG(1) != e->prim.arg_exprs[1] ? dup_expr(ctx, &e_new, e) : e;

#define BEGIN_UNOP_HANDLER() \
    struct expr e_new = *e; \
    PEVAL_ARG(0); \
    if (ARG_CONST(0)) {

#define END_UNOP_HANDLER() \
        PEVAL_ERR(e, "bad type"); \
    } \
    RETURN_MODIFIED()

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
    struct expr e_new = *e; \
    PEVAL_ARG(0); \
    PEVAL_ARG(1); \
    if (ARGS_CONST()) { \
        if (ARG(0)->t != ARG(1)->t) { \
            PEVAL_ERR(e, "type mismatch"); \
        }

#define END_BINOP_HANDLER() \
        PEVAL_ERR(e, "bad types"); \
    } \
    RETURN_MODIFIED()

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
    struct expr e_new = *e; \
    PEVAL_ARG(0); \
    PEVAL_ARG(1); \
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
    RETURN_MODIFIED()


static struct expr *peval_plus(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_UNOP(+) }
static struct expr *peval_negate(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_UNOP(-) }
static struct expr *peval_logi_not(struct peval_ctx *ctx, struct expr *e) { HANDLE_BOOL_UNOP(!) }
static struct expr *peval_bitwise_not(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_UNOP(~) }

static struct expr *peval_assign(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
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
    RETURN_MODIFIED()
}

static struct expr *peval_logi_or(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    if (ARG_CONST(0)) {
        return !bool_value(ctx, ARG(0)) ? peval(ctx, ARG(1)) : ARG(0);
    }
    RETURN_MODIFIED()
}
static struct expr *peval_logi_and(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    if (ARG_CONST(0)) {
        return bool_value(ctx, ARG(0)) ? peval(ctx, ARG(1)) : ARG(0);
    }
    RETURN_MODIFIED()
}

static struct expr *peval_bitwise_or(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_BINOP(|) }
static struct expr *peval_bitwise_xor(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_BINOP(^) }
static struct expr *peval_bitwise_and(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_BINOP(&) }

static struct expr *peval_eq(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    PEVAL_ARG(1);
    if (ARGS_CONST()) {
        RETURN_CONST(&type_bool, boolean,  const_eq(ARG(0), ARG(1)))
    }
    RETURN_MODIFIED()
}
static struct expr *peval_neq(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    PEVAL_ARG(1);
    if (ARGS_CONST()) {
        RETURN_CONST(&type_bool, boolean, !const_eq(ARG(0), ARG(1)))
    }
    RETURN_MODIFIED()
}

static struct expr *peval_lt(struct peval_ctx *ctx, struct expr *e) { HANDLE_CMP_BINOP(<) }
static struct expr *peval_gt(struct peval_ctx *ctx, struct expr *e) { HANDLE_CMP_BINOP(>) }
static struct expr *peval_lteq(struct peval_ctx *ctx, struct expr *e) { HANDLE_CMP_BINOP(<=) }
static struct expr *peval_gteq(struct peval_ctx *ctx, struct expr *e) { HANDLE_CMP_BINOP(>=) }

static struct expr *peval_bitwise_lsh(struct peval_ctx *ctx, struct expr *e) { HANDLE_SHIFT_BINOP(<<) }
static struct expr *peval_bitwise_rsh(struct peval_ctx *ctx, struct expr *e) { HANDLE_SHIFT_BINOP(>>) }

static struct expr *peval_add(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_BINOP(+) }
static struct expr *peval_sub(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_BINOP(-) }
static struct expr *peval_mul(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_BINOP(*) }
static struct expr *peval_div(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_REAL_BINOP(/) }
static struct expr *peval_mod(struct peval_ctx *ctx, struct expr *e) { HANDLE_INT_UINT_BINOP(%) }

static struct expr *peval_dot(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
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
    RETURN_MODIFIED()
}

static struct expr *peval_assert(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    if (ARG_CONST(0)) {
        ++ctx->mod_ctx->asserts_hit;
        if (!bool_value(ctx, ARG(0))) {
            ++ctx->mod_ctx->asserts_failed;
            PEVAL_ERR(e, "assertion failure!");
        }
        return unit_create(ctx, e);
    }
    RETURN_MODIFIED()
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
static struct expr *peval_quote(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    if (ctx->force_full_expansion) {
        struct expr *res = expr_create(ctx, EXPR_CONST, e);
        res->t = &type_expr;
        res->c.expr = expr_run_visitor(ARG(0), splice_visitor, ctx, ctx->arena);
        return res;
    }
    RETURN_MODIFIED()
}

static struct expr *peval_splice(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
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
}

static struct expr *peval_print(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    PEVAL_ARG(0);
    if (ctx->force_full_expansion) {
        pretty_print(ARG(0));
        return unit_create(ctx, e);
    }
    RETURN_MODIFIED()
}

static struct expr *peval_import(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
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
    RETURN_MODIFIED()
}

static struct expr *peval_static(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    ++ctx->force_full_expansion;
    PEVAL_ARG(0);
    --ctx->force_full_expansion;
    return ARG(0);
    RETURN_MODIFIED()
}



struct expr *peval(struct peval_ctx *ctx, struct expr *e) {
    assert(ctx);
    assert(ctx->scope);
    if (!e) {
        return NULL;
    }
    struct expr *result = e;
    switch (e->kind) {
    case EXPR_CONST:    return e;
    case EXPR_SYM:      result = peval_sym(ctx, e); break;
    case EXPR_STRUCT:   result = peval_struct(ctx, e); break;
    case EXPR_SELF:     result = peval_self(ctx, e); break;
    case EXPR_FUN:      result = peval_fun(ctx, e); break;
    case EXPR_BLOCK:    result = peval_block(ctx, e); break;
    case EXPR_DEF:      result = peval_def(ctx, e); break;
    case EXPR_CALL:     result = peval_call(ctx, e); break;
    case EXPR_COND:     result = peval_if(ctx, e); break;
    case EXPR_PRIM:
        switch (e->prim.kind) {
        case PRIM_PLUS:         result = peval_plus(ctx, e); break;
        case PRIM_NEGATE:       result = peval_negate(ctx, e); break;
        case PRIM_LOGI_NOT:     result = peval_logi_not(ctx, e); break;
        case PRIM_BITWISE_NOT:  result = peval_bitwise_not(ctx, e); break;
        case PRIM_ASSIGN:       result = peval_assign(ctx, e); break;
        case PRIM_LOGI_OR:      result = peval_logi_or(ctx, e); break;
        case PRIM_LOGI_AND:     result = peval_logi_and(ctx, e); break;
        case PRIM_BITWISE_OR:   result = peval_bitwise_or(ctx, e); break;
        case PRIM_BITWISE_XOR:  result = peval_bitwise_xor(ctx, e); break;
        case PRIM_BITWISE_AND:  result = peval_bitwise_and(ctx, e); break;
        case PRIM_EQ:           result = peval_eq(ctx, e); break;
        case PRIM_NEQ:          result = peval_neq(ctx, e); break;
        case PRIM_LT:           result = peval_lt(ctx, e); break;
        case PRIM_GT:           result = peval_gt(ctx, e); break;
        case PRIM_LTEQ:         result = peval_lteq(ctx, e); break;
        case PRIM_GTEQ:         result = peval_gteq(ctx, e); break;
        case PRIM_BITWISE_LSH:  result = peval_bitwise_lsh(ctx, e); break;
        case PRIM_BITWISE_RSH:  result = peval_bitwise_rsh(ctx, e); break;
        case PRIM_ADD:          result = peval_add(ctx, e); break;
        case PRIM_SUB:          result = peval_sub(ctx, e); break;
        case PRIM_MUL:          result = peval_mul(ctx, e); break;
        case PRIM_DIV:          result = peval_div(ctx, e); break;
        case PRIM_MOD:          result = peval_mod(ctx, e); break;
        case PRIM_DOT:          result = peval_dot(ctx, e); break;
        case PRIM_ASSERT:       result = peval_assert(ctx, e); break;
        case PRIM_QUOTE:        result = peval_quote(ctx, e); break;
        case PRIM_SPLICE:       result = peval_splice(ctx, e); break;
        case PRIM_PRINT:        result = peval_print(ctx, e); break;
        case PRIM_IMPORT:       result = peval_import(ctx, e); break;
        case PRIM_STATIC:       result = peval_static(ctx, e); break;
        default: assert(0 && "unknown primitive kind"); break;
        }
        break;
    default:
        assert(0 && "unknown expression kind");
        break;
    }
    if (ctx->force_full_expansion && result->kind != EXPR_CONST) {
        PEVAL_ERR(e, "expected static expression");
    }
    return result;
}



void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx) {
    memset(ctx, 0, sizeof(struct peval_ctx));

    ctx->arena = &mod_ctx->arena.a;
    ctx->global_ctx = mod_ctx->global_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->err_ctx = &mod_ctx->err_ctx;
    ctx->sym_lambda = intern_string(&mod_ctx->global_ctx->symbol_table, "lambda");
    ctx->scope = mod_ctx->global_ctx->global_scope;
    assert(ctx->scope);
}
