#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);


static struct expr *peval_no_expand(struct peval_ctx *ctx, struct expr *e) {
    uint old_force = ctx->force_full_expansion;
    ctx->force_full_expansion = 0;
    struct expr *result = peval(ctx, e);
    ctx->force_full_expansion = old_force;
    return result;
}

static struct expr *peval_force_expand(struct peval_ctx *ctx, struct expr *e, bool force_expand) {
    if (!force_expand) {
        return peval(ctx, e);
    }
    ++ctx->force_full_expansion;
    struct expr *result = peval(ctx, e);
    --ctx->force_full_expansion;
    return result;
}

static struct expr *peval_type(struct peval_ctx *ctx, struct expr *e) {
    if (!e) {
        return e;
    }
    
    struct expr *result = peval_force_expand(ctx, e, true);

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

static bool is_const_fun(struct expr *e) {
    return e && e->kind == EXPR_CONST && e->t->kind == TYPE_FUN;
}

static struct expr_decl *add_single_decl_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    struct scope *scope = ctx->scope;
    assert(scope);
    assert(d->name_expr);
    assert(d->name_expr->kind == EXPR_SYM);

    struct expr *type_expr = peval_type(ctx, d->type_expr);
    check_type(ctx, d->value_expr, type_expr);

    struct expr_decl *d_new = allocate(ctx->arena, sizeof(struct expr_decl));
    *d_new = *d;
    d_new->next = NULL;
    d_new->type_expr = type_expr;
    d_new->value_expr = peval_force_expand(ctx, d->value_expr, scope->kind == SCOPE_STRUCT || d->is_static);
    if (is_const_fun(d_new->value_expr)) {
        struct function *func = d_new->value_expr->c.fun;
        if (func->name == ctx->sym_lambda) {
            func->name = d->name_expr->sym;
        }
    }
    *scope->last_decl_ptr = d_new;
    scope->last_decl_ptr = &d_new->next;
    return d_new;
}

static void add_decls_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    for (; d; d = d->next) {
        add_single_decl_to_scope(ctx, d);
    }
}

static void add_args_to_scope(struct peval_ctx *ctx, struct expr_decl *p, struct expr_link *a) {
    for (; a; a = a->next, p = p->next) {
        assert(p);
        assert(!p->value_expr);
        assert(a->expr);
        check_type(ctx, a->expr, p->type_expr);
        struct expr_decl d = *p;
        d.value_expr = a->expr;
        add_single_decl_to_scope(ctx, &d);
    }
}

static struct expr_link *peval_args(struct peval_ctx *ctx, struct expr_link *a) {
    if (a) {
        struct expr_link a_new = *a;
        a_new.expr = peval(ctx, a->expr);
        a_new.next = peval_args(ctx, a->next);
        if (a_new.expr != a->expr || a_new.next != a->next) {
            return dup_link(ctx, &a_new);
        }
    }
    return a;
}

static struct expr_link *strip_const_args(struct peval_ctx *ctx, struct expr_link *a) {
    if (a) {
        if (a->expr->kind == EXPR_CONST) {
            return strip_const_args(ctx, a->next);
        }
        else {
            struct expr_link a_new = *a;
            a_new.next = strip_const_args(ctx, a->next);
            if (a_new.next != a->next) {
                return dup_link(ctx, &a_new);
            }
        }
    }
    return a;
}

static struct expr_decl *strip_const_params(struct peval_ctx *ctx, struct expr_decl *p, struct expr_link *a) {
    assert((a && p) || (!a && !p));
    if (p) {
        if (a->expr->kind == EXPR_CONST) {
            return strip_const_params(ctx, p->next, a->next);
        }
        else {
            struct expr_decl p_new = *p;
            p_new.next = strip_const_params(ctx, p->next, a->next);
            if (p_new.next != p->next) {
                return dup_decl(ctx, &p_new);
            }
        }
    }
    return p;
}

#define FNV1A(location, seed) fnv1a((unsigned char *)&(location), sizeof(location), seed)

static uint const_args_hash(struct expr_link *a, uint hash) {
    if (a) {
        struct expr *e = a->expr;
        if (e->kind == EXPR_CONST) {
            hash = FNV1A(e->t->kind, hash);
            switch (e->t->kind) {
            case TYPE_TYPE: hash = FNV1A(e->c.type, hash); break; /* WRONG? */
            case TYPE_BOOL: hash = FNV1A(e->c.boolean, hash); break;
            case TYPE_INT: hash = FNV1A(e->c.integer, hash); break;
            case TYPE_UINT: hash = FNV1A(e->c.uinteger, hash); break;
            case TYPE_REAL: hash = FNV1A(e->c.real, hash); break;
            case TYPE_EXPR: hash = FNV1A(e->c.expr, hash); break; /* hash expr pointer */
            default: assert(0 && "hashing unimplemented for type"); break;
            }
        } else {
            uint32_t non_const_marker = 1;
            FNV1A(non_const_marker, hash);
        }
        hash = const_args_hash(a->next, hash);
    }
    return hash;
}

static bool const_args_eq(struct peval_ctx *ctx, struct expr_link *a, struct expr_link *b) {
    if (!a || !b) {
        return !a && !b;
    }
    if (a->expr->kind == EXPR_CONST && b->expr->kind == EXPR_CONST) {
        if (!const_eq(ctx, a->expr, b->expr)) {
            return false;
        }
    } else {
        if (a->expr->kind == EXPR_CONST || b->expr->kind == EXPR_CONST) {
            return false;
        }
    }
    return const_args_eq(ctx, a->next, b->next);
}

static struct function *create_function(struct peval_ctx *ctx) {
    struct function *func = allocate(ctx->arena, sizeof(struct function));
    hashtable_init(SPECIALIZATIONS_TABLE, func->specializations, ctx->arena, 0);
    return func;
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_CALL);
    struct expr e_new = *e;
    e_new.call.callable_expr = peval(ctx, e->call.callable_expr);
    e_new.call.args = peval_args(ctx, e->call.args);
    
    const uint arg_count = expr_list_length(e_new.call.args);
    uint const_arg_count = 0;
    uint param_count = 0;
    uint static_param_count = 0;
    struct function *func = NULL;

    if (e_new.call.callable_expr->kind == EXPR_CONST) {
        if (e_new.call.callable_expr->t->kind != TYPE_FUN) {
            PEVAL_ERR(e_new.call.callable_expr, "expected a function");
        }
        func = e_new.call.callable_expr->c.fun;
        assert(func);
        assert(func->fun_expr);
        assert(func->fun_expr->kind == EXPR_FUN);
        
        param_count = decl_list_length(func->fun_expr->fun.params);
        if (arg_count != param_count) {
            PEVAL_ERR(e, "function expected %u arguments; %d were provided", param_count, arg_count);
        }

        struct expr_decl *d = func->fun_expr->fun.params;
        struct expr_link *a = e_new.call.args;
        for (; d; d = d->next, a = a->next) {
            if (d->is_static) {
                if (a->expr->kind != EXPR_CONST) {
                    PEVAL_ERR(a->expr, "argument to static parameter not const");
                }
                ++static_param_count;
            }
            if (a->expr->kind == EXPR_CONST) {
                ++const_arg_count;
            } else if (ctx->force_full_expansion) {
                PEVAL_ERR(a->expr, "argument not const");
            }
        }
        assert(const_arg_count >= static_param_count);
    }

    if (!func) {
        return e_new.call.callable_expr != e->call.callable_expr || e_new.call.args != e->call.args
            ? dup_expr(ctx, &e_new, e)
            : e;
    }

    struct expr_decl *params;
    struct expr_link *args_key;
    if (static_param_count == param_count || ctx->force_full_expansion) {
        params = func->fun_expr->fun.params;
        args_key = e_new.call.args;
        e_new.call.args = NULL;
    } else if (static_param_count == 0) {
        params = NULL;
        args_key = NULL;
    } else {
        PEVAL_ERR(e, "partial specialization not implemented");
    }

    struct function *new_func;
    bool found;
    hashtable_get(SPECIALIZATIONS_TABLE, func->specializations, args_key, new_func, found);
    if (!found) {
        struct scope *prev_scope = ctx->scope;
        ctx->scope = scope_create(ctx->arena, func->env, SCOPE_FUNCTION);
        add_args_to_scope(ctx, params, args_key);

        struct expr *return_type_expr = peval(ctx, func->fun_expr->fun.return_type_expr);
        struct expr *body_expr = peval(ctx, func->fun_expr->fun.body_expr);

        ctx->scope = prev_scope;

        if (params == func->fun_expr->fun.params &&
            return_type_expr == func->fun_expr->fun.return_type_expr &&
            body_expr == func->fun_expr->fun.body_expr)
        {
            new_func = func;
        }
        else {
            new_func = create_function(ctx);
            new_func->name = func->name;
            new_func->env = func->env;
            new_func->fun_expr = expr_create(ctx, EXPR_FUN, func->fun_expr);
            new_func->fun_expr->fun.params = params;
            new_func->fun_expr->fun.return_type_expr = return_type_expr;
            new_func->fun_expr->fun.body_expr = body_expr;

            e_new.call.callable_expr = expr_create(ctx, EXPR_CONST, e_new.call.callable_expr);
            e_new.call.callable_expr->t = &type_fun;
            e_new.call.callable_expr->c.fun = new_func;
        }
        
        check_type(ctx, body_expr, return_type_expr);

        hashtable_put(SPECIALIZATIONS_TABLE, func->specializations, args_key, new_func);
    }

    if (ctx->force_full_expansion) {
        return new_func->fun_expr->fun.body_expr;
    }

    return e_new.call.callable_expr != e->call.callable_expr || e_new.call.args != e->call.args
        ? dup_expr(ctx, &e_new, e)
        : e;
}

static struct expr *peval_sym(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_SYM);

    for (struct scope *scope = ctx->scope; scope; scope = scope->parent) {
        for (struct expr_decl *d = scope->decls; d; d = d->next) {
            if (d->name_expr->sym == e->sym) {
                if (d->value_expr && d->value_expr->kind == EXPR_CONST) {
                    return dup_expr(ctx, d->value_expr, e);
                }
                return e;
            }
        }
        if (scope->self) {
            struct expr *val = type_get_attr(scope->self, e->sym);
            if (val) {
                return dup_expr(ctx, val, e);
            }
        }
    }

    PEVAL_ERR(e, "symbol not in scope");
}

static struct expr *peval_struct(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_STRUCT);
    struct expr *result = e;

    ctx->scope = scope_create(ctx->arena, ctx->scope, SCOPE_STRUCT);

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

    ctx->scope = ctx->scope->parent;

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
    result->c.type = ctx->scope->self;
    return result;
}

static struct expr *peval_def(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_DEF);
    assert(e->def.name_expr);
    assert(e->def.value_expr);
    struct expr e_new;
    bool changed;

    struct expr_decl d;
    memset(&d, 0, sizeof(struct expr_decl));
    d.name_expr = e->def.name_expr;
    d.type_expr = e->def.type_expr;
    d.value_expr = e->def.value_expr;
    d.is_static = (e->flags & EXPR_FLAG_DEF_STATIC) != 0;
    
    struct expr_decl *d_new = add_single_decl_to_scope(ctx, &d);
    if (d_new->value_expr->kind == EXPR_CONST) {
        memset(&e_new, 0, sizeof(struct expr));
        e_new.kind = EXPR_CONST;
        e_new.t = &type_unit;
        e_new.source_pos = e->source_pos;
        changed = true;
    } else {
        e_new = *e;
        e_new.def.type_expr = d_new->type_expr;
        e_new.def.value_expr = d_new->value_expr;
        changed = e_new.def.type_expr != e->def.type_expr || e_new.def.value_expr != e->def.value_expr;
    }

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_block(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_BLOCK);

    struct expr e_new = *e;
    bool changed;

    scope_create(ctx->arena, ctx->scope, SCOPE_BLOCK);

    struct expr *body = e_new.block.body_expr = peval(ctx, e->block.body_expr);
    if (body->kind != EXPR_DEF && (body->kind != EXPR_PRIM || body->prim.kind != PRIM_SEQ)) {
        e_new = *body;
        changed = true;
    } else {
        changed = body != e->block.body_expr;
    }

    ctx->scope = ctx->scope->parent;

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_fun(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_FUN);
    struct expr *result;

    struct scope *env = scope_create(ctx->arena, ctx->scope, SCOPE_FUNCTION);

    add_decls_to_scope(ctx, e->fun.params);

    if (!e->fun.body_expr) {
        /* this is a function type expression */
        peval_type(ctx, e->fun.return_type_expr);
        result = expr_create(ctx, EXPR_CONST, e);
        result->t = &type_type;
        result->c.type = &type_fun;
    } else {
        struct expr e_new = *e;
        e_new.fun.params = ctx->scope->decls;
        e_new.fun.return_type_expr = peval_type(ctx, e->fun.return_type_expr);

        if (e_new.fun.params != e->fun.params ||
            e_new.fun.return_type_expr != e->fun.return_type_expr ||
            e_new.fun.body_expr != e->fun.body_expr) {
            result = dup_expr(ctx, &e_new, e);
        } else {
            result = e;
        }

        struct function *func = create_function(ctx);
        func->name = ctx->sym_lambda;
        func->fun_expr = result;
        func->env = env->parent;

        result = expr_create(ctx, EXPR_CONST, e);
        result->t = &type_fun;
        result->c.fun = func;
    }

    ctx->scope = env->parent;

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
    if (e->kind == EXPR_CONST) {
        return e;
    }
    
    struct expr *result = e;

    switch (e->kind) {
    case EXPR_SYM:
        result = peval_sym(ctx, e);
        break;
    case EXPR_STRUCT:
        result = peval_struct(ctx, e);
        break;
    case EXPR_SELF:
        result = peval_self(ctx, e);
        break;
    case EXPR_FUN:
        result = peval_fun(ctx, e);
        break;
    case EXPR_BLOCK:
        result = peval_block(ctx, e);
        break;
    case EXPR_DEF:
        result = peval_def(ctx, e);
        break;
    case EXPR_PRIM:
        result = peval_prim(ctx, e);
        break;
    case EXPR_CALL:
        result = peval_call(ctx, e);
        break;
    case EXPR_COND:
        result = peval_if(ctx, e);
        break;
    default:
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
}
