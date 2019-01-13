#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);


static void bind_func(struct peval_ctx *ctx, slice_t name, struct function *func) {
    slice_table_put(&ctx->mod_ctx->functions, name, func);
}

static struct function *lookup_func(struct peval_ctx *ctx, slice_t name) {
    struct function *func = NULL;
    slice_table_get(&ctx->mod_ctx->functions, name, (void **)&func);
    return func;
}

static slice_t make_func_name(struct peval_ctx *ctx, slice_t base_name) {
    uint i = 0, len = 0;
    slice_t fun_name;
    assert(base_name.len);
    fun_name.ptr = malloc(base_name.len + 9);
    memcpy(fun_name.ptr, base_name.ptr, base_name.len);
    while (1) {
        fun_name.len = base_name.len + len;
        if (!lookup_func(ctx, fun_name)) {
            break;
        }
        len = snprintf(fun_name.ptr + base_name.len, 8, "%d", i++);
    }
    return fun_name;
}


#define BEGIN_SCOPE(Kind) \
    { \
        struct scope *_prev_scope = ctx->scope; \
        struct scope _scope = { 0, }; \
        _scope.kind = Kind; \
        _scope.last_decl_ptr = &_scope.decls; \
        if (Kind == SCOPE_FUNCTION) { _scope.nearest_function_scope = &_scope; } \
        else if (_prev_scope) { _scope.nearest_function_scope = _prev_scope->nearest_function_scope; } \
        if (_prev_scope) { _scope.depth = _prev_scope->depth + 1; } \
        _scope.outer_scope = _prev_scope; \
        ctx->scope = &_scope;

#define END_SCOPE() \
        ctx->scope = _prev_scope; \
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

    if (result->expr != EXPR_CONST) {
        PEVAL_ERR(result, "expected constant type expression");
    }
    if (result->c.type->type != TYPE_TYPE) {
        PEVAL_ERR(result, "expected type expression");
    }
    return result;
}

static void check_type(struct peval_ctx *ctx, struct expr *e, struct expr *e_type) {
    if (!e || !e_type || e->expr != EXPR_CONST || e_type->expr != EXPR_CONST) {
        return;
    }
    assert(e_type->c.type->type == TYPE_TYPE);
    if (e_type->c.typeval != e->c.type) {
        PEVAL_ERR(e, "type mismatch. expected %s. found %s",
            type_names[e_type->c.typeval->type],
            type_names[e->c.type->type]);
    }
}

static bool lookup_name(struct peval_ctx *ctx, slice_t name, uint name_hash, struct expr_decl **decl_out, struct scope **scope_out) {
    struct scope *scope = ctx->scope;
    assert(scope);

    while (scope) {
        for (struct expr_decl *d = scope->decls; d; d = d->next) {
            if (d->name_expr->sym.name_hash != name_hash) {
                continue;
            }
            if (slice_equals(name, d->name_expr->sym.name)) {
                *decl_out = d;
                *scope_out = scope;
                return true;
            }
        }

        scope = scope->outer_scope;
    }

    return false;
}

static struct expr_decl *lookup_name_shallow(struct peval_ctx *ctx, slice_t name, uint name_hash) {
    for (struct expr_decl *d = ctx->scope->decls; d; d = d->next) {
        if (d->name_expr->sym.name_hash == name_hash && slice_equals(name, d->name_expr->sym.name)) {
            return d;
        }
    }
    return NULL;
}


static bool is_fun_const(struct expr *e) {
    return e && e->expr == EXPR_CONST && e->c.type->type == TYPE_FUN;
}
static bool is_dummy_fun_const(struct expr *e) {
    return is_fun_const(e) && !e->c.fun.func->fun_expr;
}

static void add_single_decl_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    struct scope *scope = ctx->scope;
    assert(scope);
    assert(d->name_expr);
    assert(d->name_expr->expr == EXPR_SYM);

    struct expr *type_expr = peval_type(ctx, d->type_expr);

    struct expr_decl *d_old = lookup_name_shallow(ctx, d->name_expr->sym.name, d->name_expr->sym.name_hash);
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
        struct expr *value_expr = peval_force_expand(ctx, d->value_expr, scope->kind == SCOPE_STATIC || d->is_static);
        check_type(ctx, value_expr, d_old->type_expr);
        
        if (!d_old->value_expr) {
            d_old->value_expr = value_expr;
        } else {
            assert(is_fun_const(value_expr));
            struct function *old_func = d_old->value_expr->c.fun.func;
            struct function *new_func = value_expr->c.fun.func;
            if (new_func->free_var_syms) {
                PEVAL_ERR(new_func->fun_expr, "static function cannot have free variables");
            }
            old_func->fun_expr = new_func->fun_expr;
        }
    } else {
        check_type(ctx, d->value_expr, type_expr);

        struct expr_decl *d_new = calloc(1, sizeof(struct expr_decl));
        *d_new = *d;
        d_new->next = NULL;
        d_new->type_expr = type_expr;
        *scope->last_decl_ptr = d_new;
        scope->last_decl_ptr = &d_new->next;

        if ((scope->kind == SCOPE_STATIC || d->is_static) && type_expr && type_expr->c.typeval->type == TYPE_FUN) {
            /* create a dummy forward declare function */
            struct function *func = calloc(1, sizeof(struct function));
            func->name = make_func_name(ctx, d->name_expr->sym.name);

            d_new->value_expr = calloc(1, sizeof(struct expr));
            d_new->value_expr->expr = EXPR_CONST;
            d_new->value_expr->c.type = &type_fun;
            d_new->value_expr->c.fun.func = func;
            
            if (d->value_expr) {
                struct expr *func_expr = peval_force_expand(ctx, d->value_expr, true);
                assert(is_fun_const(func_expr));
                func->fun_expr = func_expr->c.fun.func->fun_expr;
            }
        } else {
            d_new->value_expr = peval_force_expand(ctx, d->value_expr, scope->kind == SCOPE_STATIC || d->is_static);
            if (is_fun_const(d_new->value_expr)) {
                d_new->value_expr->c.fun.func->name = d->name_expr->sym.name;
            }
        }
    }
}

static void add_decls_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    for (; d; d = d->next) {
        add_single_decl_to_scope(ctx, d);
    }
}

static void add_consts_to_scope(struct peval_ctx *ctx, struct expr_decl *d) {
    for (; d; d = d->next) {
        assert(!d->type_expr);
        assert(d->value_expr);
        assert(d->value_expr->expr == EXPR_CONST);
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
        if (a->expr->expr == EXPR_CONST) {
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
        if (a->expr->expr == EXPR_CONST) {
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
        if (a->expr->expr == EXPR_CONST) {
            hash = fnv1a((unsigned char *)p->name_expr->sym.name.ptr, p->name_expr->sym.name.len, hash);
            hash = FNV1A(a->expr->c.type->type, hash);
            switch (a->expr->c.type->type) {
            case TYPE_TYPE: hash = FNV1A(a->expr->c.typeval, hash); break; /* WRONG? */
            case TYPE_BOOL: hash = FNV1A(a->expr->c._bool, hash); break;
            case TYPE_INT: hash = FNV1A(a->expr->c._int, hash); break;
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
        if (d->value_expr && d->value_expr->expr == EXPR_CONST) {
            ++const_count;
        }
    }
    return const_count;
}

static slice_t function_name_with_hashed_const_args(struct peval_ctx *ctx, struct function *func, struct expr_link *args) {
    uint hash = hash_const_args(func->fun_expr->fun.params, args, FNV_SEED);
    uint fun_name_capacity = func->name.len + 8;
    slice_t fun_name_new;
    fun_name_new.ptr = malloc(fun_name_capacity + 1);
    memcpy(fun_name_new.ptr, func->name.ptr, func->name.len);
    fun_name_new.len = func->name.len + snprintf(fun_name_new.ptr + func->name.len, 8, "_%u", hash);
    return fun_name_new;
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CALL);
    struct expr e_new = *e;
    e_new.call.callable_expr = peval(ctx, e->call.callable_expr);
    e_new.call.args = peval_args(ctx, e->call.args);
    
    const uint arg_count = expr_list_length(e_new.call.args);
    uint const_arg_count = 0;
    uint param_count = 0;
    uint static_param_count = 0;
    struct function *func = NULL;

    if (e_new.call.callable_expr->expr == EXPR_CONST) {
        if (e_new.call.callable_expr->c.type->type != TYPE_FUN) {
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
                    if (a->expr->expr != EXPR_CONST) {
                        PEVAL_ERR(a->expr, "argument to static parameter not const");
                    }
                    ++static_param_count;
                }
                if (a->expr->expr == EXPR_CONST) {
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
    ctx->scope->outer_scope = NULL;
    ctx->scope->depth = 0;

    add_consts_to_scope(ctx, e_new.call.callable_expr->c.fun.captured_consts);
    add_args_to_scope(ctx, func->fun_expr->fun.params, e_new.call.args);

    if (const_arg_count == param_count) {
        e_new = *peval(ctx, func->fun_expr->fun.body_expr);
        check_type(ctx, &e_new, func->fun_expr->fun.return_type_expr);
    }
    else {
        slice_t func_name_new = function_name_with_hashed_const_args(ctx, func, e_new.call.args);
        struct function *new_func = lookup_func(ctx, func_name_new);
        if (!new_func) {
            new_func = calloc(1, sizeof(struct function));
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
        e_new.call.callable_expr->c.type = &type_fun;
        e_new.call.callable_expr->c.fun.func = new_func;
        e_new.call.args = strip_const_args(ctx, e_new.call.args);
    }

    END_SCOPE();

    return dup_expr(ctx, &e_new, e);
}

static struct expr *peval_sym(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_SYM);

    struct expr_decl *d;
    struct scope *scope;
    if (!lookup_name(ctx, e->sym.name, e->sym.name_hash, &d, &scope)) {
        PEVAL_ERR(e, "symbol not in scope");
    }

    if (d->value_expr && d->value_expr->expr == EXPR_CONST) {
        return dup_expr(ctx, d->value_expr, e);
    }

    if (scope->kind == SCOPE_STATIC) {
        PEVAL_ERR(d->value_expr, "could not fully evaluate expected static expression");
    }

    struct scope *fun_scope = ctx->scope->nearest_function_scope;
    assert(fun_scope);
    if (scope->depth < fun_scope->depth) {
        e->sym.next = fun_scope->closure_syms;
        fun_scope->closure_syms = e;
    }

    return e;
}

static struct expr *peval_struct(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_STRUCT);
    struct expr e_new = *e;

    BEGIN_SCOPE(SCOPE_STATIC);

    add_decls_to_scope(ctx, e->struc.fields);

    e_new.struc.fields = ctx->scope->decls;

    END_SCOPE();

    if (e_new.struc.fields != e->struc.fields) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_let(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_LET);
    struct expr e_new = *e;
    int changed;

    BEGIN_SCOPE(SCOPE_LOCAL);

    add_decls_to_scope(ctx, e->let.bindings);

    e_new.let.bindings = ctx->scope->decls;
    changed = e_new.let.bindings != e->let.bindings;

    if (count_const_decls(e_new.let.bindings) == decl_list_length(e_new.let.bindings)) {
        e_new = *peval(ctx, e->let.body_expr);
        changed = 1;
    }
    else {
        e_new.let.body_expr = peval(ctx, e->let.body_expr);
        changed = changed || e_new.let.body_expr != e->let.body_expr;
    }

    END_SCOPE();

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_fun(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_FUN);
    struct expr *result;

    if (!e->fun.body_expr) {
        /* this is a function type expression */
        result = expr_create(ctx, EXPR_CONST, e);
        result->c.type = &type_type;
        result->c.typeval = &type_fun;
        return result;
    }

    bool is_closure;
    struct function *func;

    BEGIN_SCOPE(SCOPE_FUNCTION);

    add_decls_to_scope(ctx, e->fun.params);

    struct expr new_e = *e;
    new_e.fun.params = ctx->scope->decls;
    new_e.fun.return_type_expr = peval_type(ctx, e->fun.return_type_expr);
    check_type(ctx, new_e.fun.body_expr, new_e.fun.return_type_expr);

    new_e.fun.body_expr = peval_no_expand(ctx, e->fun.body_expr);
    check_type(ctx, new_e.fun.body_expr, new_e.fun.return_type_expr);

    func = calloc(1, sizeof(struct function));
    func->name = slice_from_str("lambda");
    func->fun_expr = dup_expr(ctx, &new_e, e);

    struct scope *scope = ctx->scope;
    struct scope *outer_scope = scope->outer_scope;
    struct scope *outer_fun_scope = outer_scope ? outer_scope->nearest_function_scope : NULL;

    struct expr *sym = scope->closure_syms;
    while (sym) {
        assert(sym->expr == EXPR_SYM);
        struct expr *next = sym->sym.next;

        struct expr_decl *d;
        struct scope *scope;
        bool found = lookup_name(ctx, sym->sym.name, sym->sym.name_hash, &d, &scope);
        assert(found);

        struct expr *new_sym = dup_expr(ctx, sym, sym);
        new_sym->sym.next = func->free_var_syms;
        func->free_var_syms = new_sym;

        if (outer_fun_scope && scope->depth < outer_fun_scope->depth) {
            sym->sym.next = outer_fun_scope->closure_syms;
            outer_fun_scope->closure_syms = sym;
        }
        else {
            sym->sym.next = NULL;
        }

        sym = next;
    }

    is_closure = !!scope->closure_syms;
    scope->closure_syms = NULL;
    END_SCOPE();

    if (!is_closure) {
        /* the identify stage determined that this wasn't a closure, so return a constant function */
        result = expr_create(ctx, EXPR_CONST, e);
        result->c.type = &type_fun;
        result->c.fun.func = func;
    }
    else {
        /* return a closure constructor */
        result = expr_create(ctx, EXPR_CAP, e);
        result->cap.func = func;
    }
    return result;
}

static struct expr *peval_cap(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CAP);
    struct function *func = e->cap.func;
    assert(func);

    struct expr_decl *captured_consts = NULL;
    struct expr *sym = func->free_var_syms;
    while (sym) {
        struct expr *temp = peval_sym(ctx, sym);
        if (temp->expr == EXPR_CONST) {
            struct expr_decl *d = calloc(1, sizeof(struct expr_decl));
            d->name_expr = sym;
            d->value_expr = temp;
            d->next = captured_consts;
            captured_consts = d;
        }
        else {
            captured_consts = NULL;
            break;
        }
        sym = sym->sym.next;
    }

    if (!captured_consts) {
        return e;
    }

    struct expr *result = expr_create(ctx, EXPR_CONST, e);
    result->c.type = &type_fun;
    result->c.fun.func = func;
    result->c.fun.captured_consts = captured_consts;
    return result;
}

static struct expr *peval_if(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_IF);
    struct expr e_new = *e;

    struct expr *cond_expr = e_new._if.cond_expr = peval(ctx, e->_if.cond_expr);

    if (cond_expr->expr == EXPR_CONST) {
        if (cond_expr->c.type->type != TYPE_BOOL) {
            PEVAL_ERR(cond_expr, "if conditional must be boolean");
        }
        e_new = *peval(ctx, cond_expr->c._bool ? e->_if.then_expr : e->_if.else_expr);
        return dup_expr(ctx, &e_new, e);
    }

    e_new._if.then_expr = peval(ctx, e->_if.then_expr);
    e_new._if.else_expr = peval(ctx, e->_if.else_expr);

    if (e_new._if.then_expr->expr == EXPR_CONST && e_new._if.else_expr->expr == EXPR_CONST &&
        (e_new._if.then_expr->c.type != e_new._if.else_expr->c.type)) {
        PEVAL_ERR(&e_new, "mismatching types in if arms");
    }

    if (e_new._if.cond_expr != e->_if.cond_expr ||
        e_new._if.then_expr != e->_if.then_expr ||
        e_new._if.else_expr != e->_if.else_expr) {
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

    switch (e->expr) {
    case EXPR_SYM:
        result = peval_sym(ctx, e);
        break;
    case EXPR_STRUCT:
        result = peval_struct(ctx, e);
        break;
    case EXPR_FUN:
        result = peval_fun(ctx, e);
        break;
    case EXPR_CAP:
        result = peval_cap(ctx, e);
        break;
    case EXPR_LET:
        result = peval_let(ctx, e);
        break;
    case EXPR_PRIM:
        result = peval_prim(ctx, e);
        break;
    case EXPR_CALL:
        result = peval_call(ctx, e);
        break;
    case EXPR_IF:
        result = peval_if(ctx, e);
        break;
    default:
        break;
    }

    if (ctx->force_full_expansion && result->expr != EXPR_CONST) {
        PEVAL_ERR(e, "expected static expression");
    }
    return result;
}

static void bind_type(struct peval_ctx *ctx, char *name_str, struct type *type) {
    struct expr *e_type = expr_create(ctx, EXPR_CONST, NULL);
    e_type->c.type = &type_type;
    e_type->c.typeval = type;

    struct expr *name_expr = expr_create(ctx, EXPR_SYM, NULL);
    name_expr->sym.name = slice_from_str(name_str);
    name_expr->sym.name_hash = slice_hash_fnv1a(name_expr->sym.name);

    struct expr_decl d = {
        .name_expr = name_expr,
        .value_expr = e_type
    };

    add_single_decl_to_scope(ctx, &d);
}

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->err_ctx = err_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->scope = &ctx->root_scope;
    ctx->scope->last_decl_ptr = &ctx->scope->decls;

    bind_type(ctx, "Expr", &type_expr);
    bind_type(ctx, "Type", &type_type);
    bind_type(ctx, "Unit", &type_unit);
    bind_type(ctx, "Bool", &type_bool);
    bind_type(ctx, "Int", &type_int);
}
