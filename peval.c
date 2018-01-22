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

static slice_t make_fn_name(struct peval_ctx *ctx) {
    uint i = 0, len = 0;
    slice_t fn_name;
    fn_name.ptr = malloc(ctx->closest_name.len + 9);
    memcpy(fn_name.ptr, ctx->closest_name.ptr, ctx->closest_name.len);
    while (1) {
        fn_name.len = ctx->closest_name.len + len;
        if (!lookup_func(ctx, fn_name)) {
            break;
        }
        len = snprintf(fn_name.ptr + ctx->closest_name.len, 8, "%d", i++);
    }
    return fn_name;
}


static struct scope *new_scope(struct peval_ctx *ctx, enum scope_kind kind) {
    struct scope *scope = calloc(1, sizeof(struct scope));
    scope->kind = kind;
    if (kind == SCOPE_FUNCTION && ctx->scope) {
        scope->function_depth = ctx->scope->function_depth + 1;
    }
    scope->outer_scope = ctx->scope;
    ctx->scope = scope;
    return scope->outer_scope; /* return old scope */
}

static struct scope *replace_scope(struct peval_ctx *ctx, struct scope *scope) {
    struct scope *prev_scope = ctx->scope;
    ctx->scope = scope;
    return prev_scope;
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

static void push_binding(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    struct scope *scope = ctx->scope;
    assert(scope);

    if (scope->num_bindings == scope->max_bindings) {
        scope->max_bindings = scope->max_bindings ? scope->max_bindings * 2 : 4;
        scope->bindings = realloc(scope->bindings, sizeof(struct binding) * scope->max_bindings);
    }
    
    struct binding *b = scope->bindings + scope->num_bindings++;
    memset(b, 0, sizeof(struct binding));
    b->name = name;
    b->name_hash = slice_hash_fnv1a(name);
    b->expr = e;
    b->scope = scope;
}

static uint push_decls_returning_count(struct peval_ctx *ctx, struct expr_decl *decls) {
    uint count = 0;
    for (struct expr_decl *d = decls; d; d = d->next) {
        push_binding(ctx, d->name, d->value_expr);
        ++count;
    }
    return count;
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
    if (!e) {
        return NULL;
    }
    
    ++ctx->force_full_expansion;
    struct expr *e_new = peval(ctx, e);
    --ctx->force_full_expansion;

    if (e_new->expr != EXPR_CONST) {
        PEVAL_ERR(e_new, "expected constant type expression");
    }
    if (e_new->u._const.type->type != TYPE_TYPE) {
        PEVAL_ERR(e_new, "expected type expression");
    }
    return e_new;
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
    uint hash = hash_const_args(func->params, args, FNV_SEED);
    uint fn_name_capacity = func->name.len + 8;
    slice_t fn_name_new;
    fn_name_new.ptr = malloc(fn_name_capacity + 1);
    memcpy(fn_name_new.ptr, func->name.ptr, func->name.len);
    fn_name_new.len = func->name.len + snprintf(fn_name_new.ptr + func->name.len, 8, "_%u", hash);
    return fn_name_new;
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    uint param_count = 0, const_count = 0;
    struct function *func = NULL;
    slice_t fn_name;

    assert(e->expr == EXPR_CALL);

    struct expr *name_expr = e_new.u.call.fn_expr = peval(ctx, e->u.call.fn_expr);
    e_new.u.call.args = peval_args(ctx, e->u.call.args, &const_count);
    int changed = name_expr != e->u.call.fn_expr || e_new.u.call.args != e->u.call.args;

    if (name_expr->expr == EXPR_CONST) {
        if (name_expr->u._const.type->type != TYPE_FN) {
            PEVAL_ERR(name_expr, "expected a function");
        }
        fn_name = name_expr->u._const.u.fn_name;
        func = lookup_func(ctx, fn_name);
        param_count = decl_list_length(func->params);
    }

    if (func && (ctx->force_full_expansion || !ctx->inhibit_call_expansion) &&
        (const_count > 0 || const_count == param_count))
    {
        struct scope *prev_scope = new_scope(ctx, SCOPE_FUNCTION);
        ctx->scope->outer_scope = NULL;
        {
            struct expr_link *a = e_new.u.call.args;
            struct expr_decl *p = func->params;
            for (; a; a = a->next, p = p->next) {
                check_type(ctx, a->expr, p->type_expr);
                push_binding(ctx, p->name, a->expr);
            }
        }

        if (const_count == param_count) {
            e_new = *peval(ctx, func->body_expr);
            check_type(ctx, &e_new, func->return_type_expr);
        }
        else {
            slice_t fn_name_new = function_name_with_hashed_const_args(ctx, func, e_new.u.call.args);

            struct function *new_func = lookup_func(ctx, fn_name_new);
            if (!new_func) {
                new_func = calloc(1, sizeof(struct function));
                *new_func = *func;
                new_func->params = strip_const_params(ctx, func->params, e_new.u.call.args);
                bind_func(ctx, fn_name_new, new_func);
                new_func->body_expr = peval(ctx, func->body_expr);
            }

            struct expr *new_name_expr = expr_create(ctx, EXPR_CONST, name_expr);
            new_name_expr->u._const.type = &type_fn;
            new_name_expr->u._const.u.fn_name = fn_name_new;

            e_new.u.call.fn_expr = new_name_expr;
            e_new.u.call.args = strip_const_args(ctx, e_new.u.call.args);
        }

        ctx->scope = prev_scope;
        changed = 1;
    }

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}


static void peval_binding(struct peval_ctx *ctx, struct binding *b) {
    assert(b);
    if (!b->expr || b->expr->expr == EXPR_CONST) {
        return;
    }

    struct scope *prev_scope = replace_scope(ctx, b->scope);
    slice_t prev_name = ctx->closest_name;
    ctx->closest_name = b->name;

    b->expr = peval(ctx, b->expr);

    ctx->closest_name = prev_name;
    ctx->scope = prev_scope;
}

static uint peval_scope_bindings_returning_const_count(struct peval_ctx *ctx) {
    uint const_count = 0;
    struct scope *scope = ctx->scope;
    for (uint i = 0; i < scope->num_bindings; ++i) {
        struct binding *b = scope->bindings + i;
        peval_binding(ctx, b);
        if (b->expr && b->expr->expr == EXPR_CONST) {
            ++const_count;
        }
    }
    return const_count;
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

    if (b->scope->function_depth < ctx->scope->function_depth) {
        ++ctx->closed_var_count;
    }

    return e;
}

static void peval_func(struct peval_ctx *ctx, struct function *func) {
    struct scope *prev_scope = new_scope(ctx, SCOPE_FUNCTION);
    uint prev_closed_var_count = ctx->closed_var_count;
    ctx->closed_var_count = 0;

    push_decls_returning_count(ctx, func->params);
    func->params = read_back_values_and_peval_types(ctx, func->params);
    func->return_type_expr = peval_type(ctx, func->return_type_expr);
    ++ctx->inhibit_call_expansion;
    func->body_expr = peval(ctx, func->body_expr);
    --ctx->inhibit_call_expansion;

    uint closed_var_count = ctx->closed_var_count;
    ctx->closed_var_count = prev_closed_var_count;
    ctx->scope = prev_scope;
    printf("closed_var_count: %d\n", closed_var_count);
}

static void begin_delay_peval_func(struct peval_ctx *ctx) {
    assert(!ctx->scope->delay_peval_func);
    ctx->scope->delay_peval_func = true;
}

static void end_delay_peval_func(struct peval_ctx *ctx) {
    struct scope *scope = ctx->scope;
    assert(scope->delay_peval_func);
    scope->delay_peval_func = false;

    struct function *func = scope->delayed_funcs;
    scope->delayed_funcs = NULL;
    while (func) {
        peval_func(ctx, func);
        struct function *next = func->next;
        func->next = NULL;
        func = next;
    }
}

struct expr *peval(struct peval_ctx *ctx, struct expr *e) {
    if (!e) {
        return NULL;
    }
    
    struct expr *result = e;

    switch (e->expr) {
    case EXPR_SYM:
        return peval_sym(ctx, e);
    case EXPR_STRUCT: {
        struct expr e_new = *e;

        struct scope *prev_scope = new_scope(ctx, SCOPE_STATIC);
        begin_delay_peval_func(ctx);

        push_decls_returning_count(ctx, e->u._struct.fields);
        peval_scope_bindings_returning_const_count(ctx);
        e_new.u._struct.fields = read_back_values_and_peval_types(ctx, e->u._struct.fields);

        end_delay_peval_func(ctx);
        ctx->scope = prev_scope;

        if (e_new.u._struct.fields != e->u._struct.fields) {
            result = dup_expr(ctx, &e_new, e);
        }
        break;
    }
    case EXPR_FN: {
        if (e->u.fn.body_expr) {
            struct function *func = calloc(1, sizeof(struct function));
            func->name = make_fn_name(ctx);
            func->fn_expr = e;
            func->params = e->u.fn.params;
            func->return_type_expr = e->u.fn.return_type_expr;
            func->body_expr = e->u.fn.body_expr;

            bind_func(ctx, func->name, func);

            if (!ctx->scope->delay_peval_func) {
                peval_func(ctx, func);
            }
            else {
                func->next = ctx->scope->delayed_funcs;
                ctx->scope->delayed_funcs = func;
            }

            result = expr_create(ctx, EXPR_CONST, e);
            result->u._const.type = &type_fn;
            result->u._const.u.fn_name = func->name;
        }
        else {
            result = expr_create(ctx, EXPR_CONST, e);
            result->u._const.type = &type_type;
            result->u._const.u.type = &type_fn;
        }
        break;
    }
    case EXPR_LET: {
        struct expr e_new = *e;
        int changed = 0;

        struct scope *prev_scope = new_scope(ctx, SCOPE_LOCAL);
        begin_delay_peval_func(ctx);

        uint total_count = push_decls_returning_count(ctx, e->u.let.bindings);
        uint const_count = peval_scope_bindings_returning_const_count(ctx);
        e_new.u.let.bindings = read_back_values_and_peval_types(ctx, e->u.let.bindings);
        changed = changed || e_new.u.let.bindings != e->u.let.bindings;

        if (const_count == total_count) {
            e_new = *peval(ctx, e->u.let.body_expr);
            changed = 1;
        }
        else {
            e_new.u.let.body_expr = peval(ctx, e->u.let.body_expr);
            changed = changed || e_new.u.let.body_expr != e->u.let.body_expr;
        }

        end_delay_peval_func(ctx);
        ctx->scope = prev_scope;

        if (changed) {
            result = dup_expr(ctx, &e_new, e);
        }
        break;
    }
    case EXPR_PRIM:
        result = peval_prim(ctx, e);
        break;
    case EXPR_CALL:
        result = peval_call(ctx, e);
        break;
    case EXPR_IF: {
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
            result = dup_expr(ctx, &e_new, e);
        }
        break;
    }
    }

    if (ctx->force_full_expansion && result->expr != EXPR_CONST) {
        PEVAL_ERR(e, "expected static expression");
    }
    return result;
}

static void bind_type(struct peval_ctx *ctx, char *name_str, struct type *type) {
    struct expr *e_type;
    slice_t name;

    name.ptr = name_str;
    name.len = strlen(name_str);
    e_type = expr_create(ctx, EXPR_CONST, NULL);
    e_type->u._const.type = &type_type;
    e_type->u._const.u.type = type;
    push_binding(ctx, name, e_type);
}

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->err_ctx = err_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->closest_name.ptr = "root";
    ctx->closest_name.len = 4;

    new_scope(ctx, SCOPE_STATIC);
    bind_type(ctx, "Expr", &type_expr);
    bind_type(ctx, "Type", &type_type);
    bind_type(ctx, "Unit", &type_unit);
    bind_type(ctx, "Bool", &type_bool);
    bind_type(ctx, "Int", &type_int);
}
