#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);


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
    case EXPR_PRIM:     result = peval_prim(ctx, e); break;
    case EXPR_CALL:     result = peval_call(ctx, e); break;
    case EXPR_COND:     result = peval_if(ctx, e); break;
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
