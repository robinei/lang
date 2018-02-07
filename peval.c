#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);
static void process_pending_functions(struct peval_ctx *ctx);


static void bind_func(struct peval_ctx *ctx, slice_t name, struct function *func) {
    slice_table_put(&ctx->mod_ctx->functions, name, func);
}
static struct function *lookup_func(struct peval_ctx *ctx, slice_t name) {
    struct function *func = NULL;
    slice_table_get(&ctx->mod_ctx->functions, name, (void **)&func);
    return func;
}


#define STACK_BINDING_COUNT 8

#define BEGIN_SCOPE(Kind) \
    { \
        struct scope *_prev_scope = ctx->scope; \
        struct scope _scope = { 0, }; \
        struct binding _bindings_storage[STACK_BINDING_COUNT]; \
        _scope.bindings = _bindings_storage; \
        _scope.max_bindings = STACK_BINDING_COUNT; \
        _scope.stack_bindings = true; \
        _scope.kind = Kind; \
        if (Kind == SCOPE_FUNCTION) { _scope.nearest_function_scope = &_scope; } \
        else if (ctx->scope) { _scope.nearest_function_scope = ctx->scope->nearest_function_scope; } \
        if (ctx->scope) { _scope.depth = ctx->scope->depth + 1; } \
        _scope.outer_scope = ctx->scope; \
        ctx->scope = &_scope;

#define END_SCOPE() \
        if (!_scope.stack_bindings) { free(_scope.bindings); } \
        ctx->scope = _prev_scope; \
    }

static void push_binding(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    struct scope *scope = ctx->scope;
    assert(scope);

    if (scope->num_bindings == scope->max_bindings) {
        if (scope->stack_bindings) {
            assert(scope->max_bindings == STACK_BINDING_COUNT);
            scope->max_bindings *= 2;
            struct binding *new_bindings = malloc(sizeof(struct binding) * scope->max_bindings);
            memcpy(new_bindings, scope->bindings, sizeof(struct binding) * scope->num_bindings);
            scope->bindings = new_bindings;
            scope->stack_bindings = false;
        }
        else {
            scope->max_bindings = scope->max_bindings ? scope->max_bindings * 2 : 16;
            scope->bindings = realloc(scope->bindings, sizeof(struct binding) * scope->max_bindings);
        }
    }

    assert(scope->bindings);
    struct binding *b = scope->bindings + scope->num_bindings++;
    b->name = name;
    b->expr = e;
    b->scope = scope;
    b->name_hash = slice_hash_fnv1a(name);
    b->pevaled = false;
}

static struct binding *lookup_binding(struct peval_ctx *ctx, slice_t name) {
    uint name_hash = slice_hash_fnv1a(name);

    struct scope *scope = ctx->scope;
    assert(scope);

    while (scope) {
        struct binding *bindings = scope->bindings;

        for (int i = (int)scope->num_bindings - 1; i >= 0; --i) {
            struct binding *b = bindings + i;
            if (b->name_hash != name_hash) {
                continue;
            }
            if (slice_equals(name, b->name)) {
                return b;
            }
        }

        scope = scope->outer_scope;
    }

    return NULL;
}

static void peval_binding(struct peval_ctx *ctx, struct binding *b) {
    assert(b);
    if (!b->expr || b->expr->expr == EXPR_CONST || b->pevaled) {
        return;
    }

    struct scope *prev_scope = ctx->scope;
    ctx->scope = b->scope;
    slice_t prev_name = ctx->closest_name;
    ctx->closest_name = b->name;

    b->pevaled = true;
    b->expr = peval(ctx, b->expr);

    process_pending_functions(ctx);

    ctx->closest_name = prev_name;
    ctx->scope = prev_scope;
}


static void push_decls(struct peval_ctx *ctx, struct expr_decl *decls) {
    for (struct expr_decl *d = decls; d; d = d->next) {
        push_binding(ctx, d->name, d->value_expr);
    }
}

static void check_type(struct peval_ctx *ctx, struct expr *e, struct expr *e_type) {
    if (!e || !e_type || e->expr != EXPR_CONST || e_type->expr != EXPR_CONST) {
        return;
    }
    assert(e_type->u._const.type->type == TYPE_TYPE);
    if (e_type->u._const.u.type != e->u._const.type) {
        PEVAL_ERR(e, "type mismatch");
    }
}

static struct expr *peval_type(struct peval_ctx *ctx, struct expr *e) {
    if (!e || ctx->inhibit_call_expansion) {
        return e;
    }
    
    ++ctx->force_full_expansion;
    struct expr *result = peval(ctx, e);
    --ctx->force_full_expansion;

    if (result->expr != EXPR_CONST) {
        PEVAL_ERR(result, "expected constant type expression");
    }
    if (result->u._const.type->type != TYPE_TYPE) {
        PEVAL_ERR(result, "expected type expression");
    }
    return result;
}

static uint peval_scope_bindings_returning_const_count(struct peval_ctx *ctx) {
    uint const_count = 0;
    struct scope *scope = ctx->scope;
    for (uint i = 0; i < scope->num_bindings; ++i) {
        struct binding *b = scope->bindings + i;
        if (b->expr) {
            peval_binding(ctx, b);
            if (b->expr->expr == EXPR_CONST) {
                ++const_count;
            }
        }
    }
    return const_count;
}

static struct expr_decl *read_back_values_and_peval_types(struct peval_ctx *ctx, struct expr_decl *f) {
    if (f) {
        struct binding *b = lookup_binding(ctx, f->name);
        assert(b);

        struct expr_decl f_new = *f;
        f_new.type_expr = peval_type(ctx, f->type_expr);
        f_new.value_expr = b->expr;
        check_type(ctx, f_new.value_expr, f_new.type_expr);
        f_new.next = read_back_values_and_peval_types(ctx, f->next);
        if (f_new.type_expr != f->type_expr || f_new.value_expr != f->value_expr || f_new.next != f->next) {
            return dup_decl(ctx, &f_new);
        }
    }
    return f;
}


static struct expr_link *peval_args(struct peval_ctx *ctx, struct expr_link *a, uint *const_count) {
    if (a) {
        struct expr_link a_new = *a;
        a_new.expr = peval(ctx, a->expr);
        a_new.next = peval_args(ctx, a->next, const_count);
        if (a_new.expr->expr == EXPR_CONST) {
            ++*const_count;
        }
        if (a_new.expr != a->expr || a_new.next != a->next) {
            return dup_arg(ctx, &a_new);
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
                return dup_arg(ctx, &a_new);
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

static uint hash_const_args(struct expr_decl *p, struct expr_link *a, uint hash) {
    if (a) {
        assert(p);
        if (a->expr->expr == EXPR_CONST) {
            hash = fnv1a((unsigned char *)p->name.ptr, p->name.len, hash);
            switch (a->expr->u._const.type->type) {
            case TYPE_TYPE: hash = fnv1a((unsigned char *)&a->expr->u._const.type, sizeof(struct type *), hash); break;
            case TYPE_BOOL: hash = fnv1a((unsigned char *)&a->expr->u._const.u._bool, sizeof(a->expr->u._const.u._bool), hash); break;
            case TYPE_INT: hash = fnv1a((unsigned char *)&a->expr->u._const.u._int, sizeof(a->expr->u._const.u._int), hash); break;
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

static slice_t function_name_with_hashed_const_args(struct peval_ctx *ctx, struct function *func, struct expr_link *args) {
    uint hash = hash_const_args(func->fun_expr->u.fun.params, args, FNV_SEED);
    uint fun_name_capacity = func->name.len + 8;
    slice_t fun_name_new;
    fun_name_new.ptr = malloc(fun_name_capacity + 1);
    memcpy(fun_name_new.ptr, func->name.ptr, func->name.len);
    fun_name_new.len = func->name.len + snprintf(fun_name_new.ptr + func->name.len, 8, "_%u", hash);
    return fun_name_new;
}

static void process_pending_function(struct peval_ctx *ctx, struct expr *fun, struct function *func) {
    assert(fun->expr == EXPR_FUN);

    BEGIN_SCOPE(SCOPE_FUNCTION);

    push_decls(ctx, fun->u.fun.params);

    slice_t prev_name;
    if (func) {
        prev_name = ctx->closest_name;
        ctx->closest_name = func->name;
    }

    struct expr new_fun = *fun;
    new_fun.u.fun.params = read_back_values_and_peval_types(ctx, fun->u.fun.params);
    new_fun.u.fun.return_type_expr = peval_type(ctx, fun->u.fun.return_type_expr);
    new_fun.u.fun.body_expr = peval(ctx, fun->u.fun.body_expr);

    if (func) {
        ctx->closest_name = prev_name;
        func->fun_expr = dup_expr(ctx, &new_fun, fun);
        func->free_var_syms = NULL;
    }

    struct scope *scope = ctx->scope;
    struct scope *outer_scope = scope->outer_scope;
    struct scope *outer_fun_scope = outer_scope ? outer_scope->nearest_function_scope : NULL;

    process_pending_functions(ctx);

    struct expr *sym = scope->closure_syms;
    while (sym) {
        assert(sym->expr == EXPR_SYM);
        struct expr *next = sym->u.sym.next;
        struct binding *b = lookup_binding(ctx, sym->u.sym.name);
        assert(b);

        if (func) {
            struct expr *new_sym = dup_expr(ctx, sym, sym);
            new_sym->u.sym.next = func->free_var_syms;
            func->free_var_syms = new_sym;
        }

        if (outer_fun_scope && b->scope->depth < outer_fun_scope->depth) {
            sym->u.sym.next = outer_fun_scope->closure_syms;
            outer_fun_scope->closure_syms = sym;
        }
        else {
            sym->u.sym.next = NULL;
        }

        sym = next;
    }

    fun->is_closure_fun = !!scope->closure_syms;
    scope->closure_syms = NULL;
    END_SCOPE();
}

static void process_pending_functions(struct peval_ctx *ctx) {
    if (ctx->identify_closures) {
        assert(!ctx->scope->pending_functions);
        struct expr *dummy_fun = ctx->scope->pending_dummy_funs;
        while (dummy_fun) {
            assert(dummy_fun->expr == EXPR_CONST);
            assert(dummy_fun->u._const.type->type == TYPE_DUMMY_FUN);
            process_pending_function(ctx, dummy_fun->u._const.u.dummy_fun.expr, NULL);
            dummy_fun = dummy_fun->u._const.u.dummy_fun.next;
        }
    }
    else {
        assert(!ctx->scope->pending_dummy_funs);
        struct function *func = ctx->scope->pending_functions;
        while (func) {
            process_pending_function(ctx, func->fun_expr, func);
            func = func->next;
        }
    }
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    uint param_count = 0, const_count = 0;
    struct function *func = NULL;
    struct expr *fun_expr = NULL;

    assert(e->expr == EXPR_CALL);

    struct expr *callable_expr = e_new.u.call.callable_expr = peval(ctx, e->u.call.callable_expr);
    e_new.u.call.args = peval_args(ctx, e->u.call.args, &const_count);
    int changed = callable_expr != e->u.call.callable_expr || e_new.u.call.args != e->u.call.args;

    if (callable_expr->expr == EXPR_CONST && callable_expr->u._const.type->type != TYPE_DUMMY_FUN) {
        if (callable_expr->u._const.type->type != TYPE_FUN) {
            PEVAL_ERR(callable_expr, "expected a function");
        }
        func = callable_expr->u._const.u.fun.func;
        fun_expr = func->fun_expr;
        param_count = decl_list_length(fun_expr->u.fun.params);
    }

    if (!(func && (ctx->force_full_expansion || !ctx->inhibit_call_expansion) &&
        (const_count > 0 || const_count == param_count)))
    {
        return changed ? dup_expr(ctx, &e_new, e) : e;
    }

    uint arg_count = expr_list_length(e_new.u.call.args);
    if (arg_count != param_count) {
        PEVAL_ERR(e, "function expected %u arguments; %d were provided", param_count, arg_count);
    }

    BEGIN_SCOPE(SCOPE_FUNCTION);
    ctx->scope->outer_scope = NULL;
    {
        struct expr_decl *decl = callable_expr->u._const.u.fun.captured_consts;
        for (; decl; decl = decl->next) {
            push_binding(ctx, decl->name, decl->value_expr);
        }

        struct expr_link *a = e_new.u.call.args;
        struct expr_decl *p = fun_expr->u.fun.params;
        for (; a; a = a->next, p = p->next) {
            check_type(ctx, a->expr, p->type_expr);
            push_binding(ctx, p->name, a->expr);
        }
    }

    if (const_count == param_count) {
        e_new = *peval(ctx, fun_expr->u.fun.body_expr);
        check_type(ctx, &e_new, fun_expr->u.fun.return_type_expr);
    }
    else {
        slice_t fun_name_new = function_name_with_hashed_const_args(ctx, func, e_new.u.call.args);

        struct function *new_func = lookup_func(ctx, fun_name_new);
        if (!new_func) {
            new_func = calloc(1, sizeof(struct function));
            *new_func = *func;
            new_func->name = fun_name_new;

            struct expr *new_fun = expr_create(ctx, EXPR_FUN, fun_expr);
            new_fun->u.fun.params = strip_const_params(ctx, fun_expr->u.fun.params, e_new.u.call.args);
            new_fun->u.fun.return_type_expr = peval(ctx, fun_expr->u.fun.return_type_expr);
            bind_func(ctx, fun_name_new, new_func);
            new_fun->u.fun.body_expr = peval(ctx, fun_expr->u.fun.body_expr);
            new_func->fun_expr = new_fun;
        }

        struct expr *new_callable_expr = expr_create(ctx, EXPR_CONST, callable_expr);
        new_callable_expr->u._const.type = &type_fun;
        new_callable_expr->u._const.u.fun.func = new_func;

        e_new.u.call.callable_expr = new_callable_expr;
        e_new.u.call.args = strip_const_args(ctx, e_new.u.call.args);
    }

    process_pending_functions(ctx);

    END_SCOPE();

    return dup_expr(ctx, &e_new, e);
}

static struct expr *peval_sym(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_SYM);
    struct binding *b = lookup_binding(ctx, e->u.sym.name);
    if (!b) {
        return e;
    }

    if (b->expr) {
        if (b->expr->expr == EXPR_CONST) {
            return dup_expr(ctx, b->expr, e);
        }

        peval_binding(ctx, b);

        if (b->expr->expr == EXPR_CONST) {
            return dup_expr(ctx, b->expr, e);
        }
    }

    if (b->scope->kind == SCOPE_STATIC) {
        PEVAL_ERR(b->expr, "could not fully evaluate expected static expression");
    }

    struct scope *fun_scope = ctx->scope->nearest_function_scope;
    assert(fun_scope);
    if (b->scope->depth < fun_scope->depth) {
        e->u.sym.next = fun_scope->closure_syms;
        fun_scope->closure_syms = e;
    }

    return e;
}

static struct expr *peval_struct(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;

    BEGIN_SCOPE(SCOPE_STATIC);

    push_decls(ctx, e->u._struct.fields);
    peval_scope_bindings_returning_const_count(ctx);
    e_new.u._struct.fields = read_back_values_and_peval_types(ctx, e->u._struct.fields);

    END_SCOPE();

    if (e_new.u._struct.fields != e->u._struct.fields) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static struct expr *peval_let(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    int changed = 0;

    BEGIN_SCOPE(SCOPE_LOCAL);

    push_decls(ctx, e->u.let.bindings);
    uint const_count = peval_scope_bindings_returning_const_count(ctx);
    e_new.u.let.bindings = read_back_values_and_peval_types(ctx, e->u.let.bindings);
    changed = changed || e_new.u.let.bindings != e->u.let.bindings;

    if (const_count == ctx->scope->num_bindings) {
        e_new = *peval(ctx, e->u.let.body_expr);
        changed = 1;
    }
    else {
        e_new.u.let.body_expr = peval(ctx, e->u.let.body_expr);
        changed = changed || e_new.u.let.body_expr != e->u.let.body_expr;
    }

    END_SCOPE();

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

static slice_t make_fun_name(struct peval_ctx *ctx) {
    uint i = 0, len = 0;
    slice_t fun_name;
    fun_name.ptr = malloc(ctx->closest_name.len + 9);
    memcpy(fun_name.ptr, ctx->closest_name.ptr, ctx->closest_name.len);
    while (1) {
        fun_name.len = ctx->closest_name.len + len;
        if (!lookup_func(ctx, fun_name)) {
            break;
        }
        len = snprintf(fun_name.ptr + ctx->closest_name.len, 8, "%d", i++);
    }
    return fun_name;
}

static struct expr *peval_fun(struct peval_ctx *ctx, struct expr *e) {
    struct expr *result;

    if (!e->u.fun.body_expr) {
        /* this is a function type expression */
        result = expr_create(ctx, EXPR_CONST, e);
        result->u._const.type = &type_type;
        result->u._const.u.type = &type_fun;
        return result;
    }

    if (ctx->identify_closures) {
        /* assume all functions are have zero free variables and return them as dummy constants.
        later when identify_closures is false, functions which still have free
        variables will be "demoted" from a function constant to a closure constructor.
        that may in turn affect others which referenced that function, and so on */
        result = expr_create(ctx, EXPR_CONST, e);
        result->u._const.type = &type_dummy_fun;
        result->u._const.u.dummy_fun.expr = e;
        result->u._const.u.dummy_fun.next = ctx->scope->pending_dummy_funs;
        ctx->scope->pending_dummy_funs = result;
        return result;
    }

    struct function *func = calloc(1, sizeof(struct function));
    func->name = make_fun_name(ctx);
    func->fun_expr = e;
    func->next = ctx->scope->pending_functions;
    ctx->scope->pending_functions = func;
    bind_func(ctx, func->name, func);

    if (!e->is_closure_fun) {
        /* the identify stage determined that this wasn't a closure, so return a constant function */
        result = expr_create(ctx, EXPR_CONST, e);
        result->u._const.type = &type_fun;
        result->u._const.u.fun.func = func;
    }
    else {
        /* return a closure constructor */
        result = expr_create(ctx, EXPR_CAP, e);
        result->u.cap.func = func;
    }
    return result;
}

static struct expr *peval_cap(struct peval_ctx *ctx, struct expr *e) {
    struct function *func = e->u.cap.func;
    assert(func);

    struct expr_decl *captured_consts = NULL;
    struct expr *sym = func->free_var_syms;
    while (sym) {
        struct expr *temp = peval_sym(ctx, sym);
        if (temp->expr == EXPR_CONST) {
            struct expr_decl *decl = calloc(1, sizeof(struct expr_decl));
            decl->name = sym->u.sym.name;
            decl->value_expr = temp;
            decl->next = captured_consts;
            captured_consts = decl;
        }
        else {
            captured_consts = NULL;
            break;
        }
        sym = sym->u.sym.next;
    }

    if (!captured_consts) {
        return e;
    }

    struct expr *result = expr_create(ctx, EXPR_CONST, e);
    result->u._const.type = &type_fun;
    result->u._const.u.fun.func = func;
    result->u._const.u.fun.captured_consts = captured_consts;
    return result;
}

static struct expr *peval_if(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;

    e_new.u._if.cond_expr = peval(ctx, e->u._if.cond_expr);

    if (e_new.u._if.cond_expr->expr == EXPR_CONST) {
        struct expr_const *_const = &e_new.u._if.cond_expr->u._const;
        if (_const->type->type != TYPE_BOOL) {
            PEVAL_ERR(e_new.u._if.cond_expr, "if conditional must be boolean");
        }
        e_new = *peval(ctx, _const->u._bool ? e->u._if.then_expr : e->u._if.else_expr);
        return dup_expr(ctx, &e_new, e);
    }

    e_new.u._if.then_expr = peval(ctx, e->u._if.then_expr);
    e_new.u._if.else_expr = peval(ctx, e->u._if.else_expr);

    if (e_new.u._if.then_expr->expr == EXPR_CONST && e_new.u._if.else_expr->expr == EXPR_CONST &&
        (e_new.u._if.then_expr->u._const.type != e_new.u._if.else_expr->u._const.type)) {
        PEVAL_ERR(&e_new, "mismatching types in if arms");
    }

    if (e_new.u._if.cond_expr != e->u._if.cond_expr ||
        e_new.u._if.then_expr != e->u._if.then_expr ||
        e_new.u._if.else_expr != e->u._if.else_expr) {
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
    case EXPR_CONST:
        break;
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
    }

    if (ctx->force_full_expansion && result->expr != EXPR_CONST) {
        PEVAL_ERR(e, "expected static expression");
    }
    return result;
}

static void bind_type(struct peval_ctx *ctx, char *name_str, struct type *type) {
    struct expr *e_type = expr_create(ctx, EXPR_CONST, NULL);
    e_type->u._const.type = &type_type;
    e_type->u._const.u.type = type;
    push_binding(ctx, slice_from_str(name_str), e_type);
}

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->err_ctx = err_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->closest_name = slice_from_str("root");
    ctx->scope = &ctx->root_scope;

    bind_type(ctx, "Expr", &type_expr);
    bind_type(ctx, "Type", &type_type);
    bind_type(ctx, "Unit", &type_unit);
    bind_type(ctx, "Bool", &type_bool);
    bind_type(ctx, "Int", &type_int);
}
