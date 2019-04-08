#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);


static void bind_func(struct peval_ctx *ctx, struct symbol *name, struct function *func) {
    pointer_table_put(&ctx->mod_ctx->functions, name, func);
}

static struct function *lookup_func(struct peval_ctx *ctx, struct symbol *name) {
    struct function *func = NULL;
    pointer_table_get(&ctx->mod_ctx->functions, name, (void **)&func);
    return func;
}

static struct symbol *make_func_name(struct peval_ctx *ctx, struct symbol *base_name) {
    char buf[256];
    uint i = 0;
    assert(base_name->length);
    assert(base_name->length + 16 < sizeof(buf));
    memcpy(buf, base_name->data, base_name->length + 1);
    while (1) {
        struct symbol *fun_name = intern_string(&ctx->global_ctx->symbol_table, buf);
        if (!lookup_func(ctx, fun_name)) {
            return fun_name;
        }
        snprintf(buf + base_name->length, 16, "%d", i++);
    }
}


#define BEGIN_SCOPE(Kind) \
    { \
        struct scope _scope = { \
            .kind = Kind, \
            .last_decl_ptr = &_scope.decls, \
            .outer_scope = ctx->scope \
        }; \
        ctx->scope = &_scope;

#define END_SCOPE() \
        ctx->scope = _scope.outer_scope; \
    }

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
    if (result->c.tag->kind != TYPE_TYPE) {
        PEVAL_ERR(result, "expected type expression");
    }
    return result;
}

static void check_type(struct peval_ctx *ctx, struct expr *e, struct expr *e_type) {
    if (!e || !e_type || e->kind != EXPR_CONST || e_type->kind != EXPR_CONST) {
        return;
    }
    assert(e_type->c.tag->kind == TYPE_TYPE);
    if (e_type->c.type != e->c.tag) {
        PEVAL_ERR(e, "type mismatch. expected %s. found %s",
            type_names[e_type->c.type->kind],
            type_names[e->c.tag->kind]);
    }
}

static struct expr_decl *lookup_name_shallow(struct peval_ctx *ctx, struct symbol *name) {
    for (struct expr_decl *d = ctx->scope->decls; d; d = d->next) {
        if (d->name_expr->sym == name) {
            return d;
        }
    }
    return NULL;
}


static bool is_fun_const(struct expr *e) {
    return e && e->kind == EXPR_CONST && e->c.tag->kind == TYPE_FUN;
}
static bool is_dummy_fun_const(struct expr *e) {
    return is_fun_const(e) && !e->c.fun.func->fun_expr;
}

static struct expr_decl *add_single_decl_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    struct scope *scope = ctx->scope;
    assert(scope);
    assert(d->name_expr);
    assert(d->name_expr->kind == EXPR_SYM);

    struct expr *type_expr = peval_type(ctx, d->type_expr);

    struct expr_decl *d_old = lookup_name_shallow(ctx, d->name_expr->sym);
    if (d_old) {
        if (d_old->value_expr && !is_dummy_fun_const(d_old->value_expr)) {
            PEVAL_ERR(d_old->value_expr, "forward declaration cannot have value");
        }
        if (!d_old->type_expr) {
            PEVAL_ERR(d_old->name_expr, "forward declaration must have type declaration");
        }
        if (type_expr) {
            PEVAL_ERR(type_expr, "type of name was already forward declared");
        }
        if (!d->value_expr) {
            PEVAL_ERR(d->name_expr, "expected value matching forward declaration");
        }
        struct expr *value_expr = peval_force_expand(ctx, d->value_expr, scope->kind == SCOPE_STRUCT || d->is_static);
        check_type(ctx, value_expr, d_old->type_expr);
        
        if (!d_old->value_expr) {
            d_old->value_expr = value_expr;
        } else {
            assert(is_fun_const(value_expr));
            struct function *old_func = d_old->value_expr->c.fun.func;
            struct function *new_func = value_expr->c.fun.func;
            old_func->fun_expr = new_func->fun_expr;
        }
        return d_old;
    } else {
        check_type(ctx, d->value_expr, type_expr);

        struct expr_decl *d_new = allocate(ctx->arena, sizeof(struct expr_decl));
        *d_new = *d;
        d_new->next = NULL;
        d_new->type_expr = type_expr;
        *scope->last_decl_ptr = d_new;
        scope->last_decl_ptr = &d_new->next;

        if ((scope->kind == SCOPE_STRUCT || d->is_static) && type_expr && type_expr->c.type->kind == TYPE_FUN) {
            /* create a dummy forward declare function */
            struct function *func = allocate(ctx->arena, sizeof(struct function));
            func->name = make_func_name(ctx, d->name_expr->sym);

            d_new->value_expr = allocate(ctx->arena, sizeof(struct expr));
            d_new->value_expr->kind = EXPR_CONST;
            d_new->value_expr->c.tag = &type_fun;
            d_new->value_expr->c.fun.func = func;
            
            if (d->value_expr) {
                struct expr *func_expr = peval_force_expand(ctx, d->value_expr, true);
                assert(is_fun_const(func_expr));
                func->fun_expr = func_expr->c.fun.func->fun_expr;
            }
        } else {
            d_new->value_expr = peval_force_expand(ctx, d->value_expr, scope->kind == SCOPE_STRUCT || d->is_static);
            if (is_fun_const(d_new->value_expr)) {
                struct function *func = d_new->value_expr->c.fun.func;
                if (func->name == ctx->sym_lambda) {
                    func->name = d->name_expr->sym;
                }
            }
        }
        return d_new;
    }
}

static void add_decls_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    for (; d; d = d->next) {
        add_single_decl_to_scope(ctx, d);
    }
}

static void add_args_to_scope(struct peval_ctx *ctx, struct expr_decl *p, struct expr_link *a) {
    for (; a; a = a->next, p = p->next) {
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

static uint hash_const_args(struct expr_decl *p, struct expr_link *a, uint hash) {
    if (a) {
        assert(p);
        if (a->expr->kind == EXPR_CONST) {
            hash = fnv1a((unsigned char *)p->name_expr->sym->data, p->name_expr->sym->length, hash);
            hash = FNV1A(a->expr->c.tag->kind, hash);
            switch (a->expr->c.tag->kind) {
            case TYPE_TYPE: hash = FNV1A(a->expr->c.type, hash); break; /* WRONG? */
            case TYPE_BOOL: hash = FNV1A(a->expr->c.boolean, hash); break;
            case TYPE_INT: hash = FNV1A(a->expr->c.integer, hash); break;
            default: assert(0); break;
            }
        }
        hash = hash_const_args(p->next, a->next, hash);
    }
    else {
        assert(!p);
    }
    return hash;
}

static uint count_const_decls(struct expr_decl *d) {
    uint const_count = 0;
    for (; d; d = d->next) {
        if (d->value_expr && d->value_expr->kind == EXPR_CONST) {
            ++const_count;
        }
    }
    return const_count;
}

static struct symbol *function_name_with_hashed_const_args(struct peval_ctx *ctx, struct function *func, struct expr_link *args) {
    char buf[256];
    uint hash = hash_const_args(func->fun_expr->fun.params, args, FNV_SEED);
    assert(func->name->length + 16 < sizeof(buf));
    memcpy(buf, func->name->data, func->name->length + 1);
    snprintf(buf + func->name->length, 16, "_%u", hash);
    return intern_string(&ctx->global_ctx->symbol_table, buf);
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
        if (e_new.call.callable_expr->c.tag->kind != TYPE_FUN) {
            PEVAL_ERR(e_new.call.callable_expr, "expected a function");
        }
        func = e_new.call.callable_expr->c.fun.func;
        
        if (func->fun_expr) {
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
                }
            }
            assert(const_arg_count >= static_param_count);
        } else {
            /* forward declaration dummy. cannat be called now */
            func = NULL;
        }
    }

    if (!func || (!ctx->force_full_expansion && static_param_count == 0)) {
        return e_new.call.callable_expr != e->call.callable_expr || e_new.call.args != e->call.args
            ? dup_expr(ctx, &e_new, e)
            : e;
    }

    BEGIN_SCOPE(SCOPE_FUNCTION);
    struct scope *outer = ctx->scope->outer_scope;
    ctx->scope->outer_scope = NULL;

    add_args_to_scope(ctx, func->fun_expr->fun.params, e_new.call.args);

    if (const_arg_count == param_count) {
        e_new = *peval(ctx, func->fun_expr->fun.body_expr);
        check_type(ctx, &e_new, func->fun_expr->fun.return_type_expr);
    }
    else {
        struct symbol *func_name_new = function_name_with_hashed_const_args(ctx, func, e_new.call.args);
        struct function *new_func = lookup_func(ctx, func_name_new);
        if (!new_func) {
            new_func = allocate(ctx->arena, sizeof(struct function));
            *new_func = *func;
            new_func->name = func_name_new;
            new_func->fun_expr = expr_create(ctx, EXPR_FUN, func->fun_expr);
            new_func->fun_expr->fun.params = strip_const_params(ctx, func->fun_expr->fun.params, e_new.call.args);
            new_func->fun_expr->fun.return_type_expr = peval(ctx, func->fun_expr->fun.return_type_expr);
            check_type(ctx, func->fun_expr->fun.body_expr, func->fun_expr->fun.return_type_expr);
            bind_func(ctx, func_name_new, new_func);
            new_func->fun_expr->fun.body_expr = peval(ctx, func->fun_expr->fun.body_expr);
            check_type(ctx, new_func->fun_expr->fun.body_expr, new_func->fun_expr->fun.return_type_expr);
        }

        e_new.call.callable_expr = expr_create(ctx, EXPR_CONST, e_new.call.callable_expr);
        e_new.call.callable_expr->c.tag = &type_fun;
        e_new.call.callable_expr->c.fun.func = new_func;
        e_new.call.args = strip_const_args(ctx, e_new.call.args);
    }

    ctx->scope->outer_scope = outer;
    END_SCOPE();

    return dup_expr(ctx, &e_new, e);
}

static struct expr *peval_sym(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_SYM);

    struct scope *scope = ctx->scope;
    while (scope) {
        for (struct expr_decl *d = scope->decls; d; d = d->next) {
            if (d->name_expr->sym == e->sym) {
                if (d->value_expr && d->value_expr->kind == EXPR_CONST) {
                    return dup_expr(ctx, d->value_expr, e);
                }
                struct scope *found_in = scope;
                scope = ctx->scope;
                while (scope != found_in) {
                    ++scope->free_var_count;
                    scope = scope->outer_scope;
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
        scope = scope->outer_scope;
    }

    PEVAL_ERR(e, "symbol not in scope");
}

static struct expr *peval_struct(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_STRUCT);
    struct expr *result = e;

    BEGIN_SCOPE(SCOPE_STRUCT);

    struct type *self = allocate(ctx->arena, sizeof(struct type));
    pointer_table_init(&self->attrs, &ctx->mod_ctx->alloc.a, 0);
    self->kind = TYPE_STRUCT;
    ctx->scope->self = self;

    struct expr *body = peval(ctx, e->struc.body_expr);
    
    if (body->kind == EXPR_CONST) {
        result = expr_create(ctx, EXPR_CONST, e);
        result->c.tag = &type_type;
        result->c.type = self;
    }

    END_SCOPE();

    return result;
}

static struct expr *peval_self(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_SELF);
    assert(ctx->scope);
    struct type *self = NULL;
    for (struct scope *scope = ctx->scope; scope; scope = scope->outer_scope) {
        if (scope->self) {
            self = scope->self;
            break;
        }
    }
    assert(self);
    struct expr *result = expr_create(ctx, EXPR_CONST, e);
    result->c.tag = &type_type;
    result->c.type = ctx->scope->self;
    return result;
}

static struct expr *peval_def(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_DEF);
    struct expr e_new;
    bool changed;

    struct expr_decl d = {
        .name_expr = e->def.name_expr,
        .type_expr = e->def.type_expr,
        .value_expr = e->def.value_expr,
        .is_static = (e->flags & EXPR_FLAG_DEF_STATIC) != 0
    };
    struct expr_decl *d_new = add_single_decl_to_scope(ctx, &d);
    if (d_new->value_expr && d_new->value_expr->kind == EXPR_CONST) {
        memset(&e_new, 0, sizeof(struct expr));
        e_new.kind = EXPR_CONST;
        e_new.c.tag = &type_unit;
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

    BEGIN_SCOPE(SCOPE_BLOCK);

    struct expr *body = e_new.block.body_expr = peval(ctx, e->block.body_expr);
    if (body->kind != EXPR_DEF && (body->kind != EXPR_PRIM || body->prim.kind != PRIM_SEQ)) {
        e_new = *body;
        changed = true;
    } else {
        changed = body != e->block.body_expr;
    }

    END_SCOPE();

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_fun(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_FUN);
    struct expr *result;

    if (!e->fun.body_expr) {
        peval_type(ctx, e->fun.return_type_expr);
        /* this is a function type expression */
        result = expr_create(ctx, EXPR_CONST, e);
        result->c.tag = &type_type;
        result->c.type = &type_fun;
        return result;
    }

    BEGIN_SCOPE(SCOPE_FUNCTION);

    add_decls_to_scope(ctx, e->fun.params);

    struct expr e_new = *e;
    e_new.fun.params = ctx->scope->decls;
    e_new.fun.return_type_expr = peval_type(ctx, e->fun.return_type_expr);
    e_new.fun.body_expr = peval_no_expand(ctx, e->fun.body_expr);
    check_type(ctx, e_new.fun.body_expr, e_new.fun.return_type_expr);

    if (e_new.fun.params != e->fun.params ||
        e_new.fun.return_type_expr != e->fun.return_type_expr ||
        e_new.fun.body_expr != e->fun.body_expr) {
        result = dup_expr(ctx, &e_new, e);
    } else {
        result = e;
    }

    if (ctx->scope->free_var_count == 0) {
        struct function *func = allocate(ctx->arena, sizeof(struct function));
        func->name = ctx->sym_lambda;
        func->fun_expr = result;

        result = expr_create(ctx, EXPR_CONST, e);
        result->c.tag = &type_fun;
        result->c.fun.func = func;
    }

    END_SCOPE();
    return result;
}

static struct expr *peval_if(struct peval_ctx *ctx, struct expr *e) {
    assert(e->kind == EXPR_COND);
    struct expr e_new = *e;

    struct expr *pred_expr = e_new.cond.pred_expr = peval(ctx, e->cond.pred_expr);

    if (pred_expr->kind == EXPR_CONST) {
        if (pred_expr->c.tag->kind != TYPE_BOOL) {
            PEVAL_ERR(pred_expr, "if conditional must be boolean");
        }
        e_new = *peval(ctx, pred_expr->c.boolean ? e->cond.then_expr : e->cond.else_expr);
        return dup_expr(ctx, &e_new, e);
    }

    e_new.cond.then_expr = peval(ctx, e->cond.then_expr);
    e_new.cond.else_expr = peval(ctx, e->cond.else_expr);

    if (e_new.cond.then_expr->kind == EXPR_CONST && e_new.cond.else_expr->kind == EXPR_CONST &&
        (e_new.cond.then_expr->c.tag != e_new.cond.else_expr->c.tag)) {
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

    assert(expr_source_text(result).ptr);

    if (ctx->force_full_expansion && result->kind != EXPR_CONST) {
        PEVAL_ERR(e, "expected static expression");
    }
    return result;
}

static void bind_type(struct peval_ctx *ctx, char *name_str, struct type *type) {
    struct expr *e_type = expr_create(ctx, EXPR_CONST, NULL);
    e_type->c.tag = &type_type;
    e_type->c.type = type;

    struct expr *name_expr = expr_create(ctx, EXPR_SYM, NULL);
    name_expr->sym = intern_string(&ctx->global_ctx->symbol_table, name_str);

    struct expr_decl d = {
        .name_expr = name_expr,
        .value_expr = e_type
    };

    add_single_decl_to_scope(ctx, &d);
}

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx) {
    memset(ctx, 0, sizeof(struct peval_ctx));

    ctx->arena = &mod_ctx->arena.a;
    ctx->global_ctx = mod_ctx->global_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->err_ctx = &mod_ctx->err_ctx;
    ctx->scope = &ctx->root_scope;
    ctx->scope->last_decl_ptr = &ctx->scope->decls;
    ctx->sym_lambda = intern_string(&mod_ctx->global_ctx->symbol_table, "lambda");

    bind_type(ctx, "Expr", &type_expr);
    bind_type(ctx, "Type", &type_type);
    bind_type(ctx, "Unit", &type_unit);
    bind_type(ctx, "Bool", &type_bool);
    bind_type(ctx, "Int", &type_int);
    bind_type(ctx, "String", &type_string);
}
