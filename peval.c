#include "peval.h"
#include "peval_internal.h"
#include "fnv.h"
#include <stdio.h>
#include <string.h>

struct expr *peval_prim(struct peval_ctx *ctx, struct expr *e);

struct peval_binding {
    slice_t name;
    struct expr *expr;
};

static struct expr missing;

static void push_name(struct peval_ctx *ctx, slice_t name) {
    assert(ctx->name_stack_count < NAME_STACK_SIZE);
    ctx->name_stack[ctx->name_stack_count++] = ctx->closest_name;
    ctx->closest_name = name;
}

static void pop_name(struct peval_ctx *ctx) {
    assert(ctx->name_stack_count > 0);
    ctx->closest_name = ctx->name_stack[--ctx->name_stack_count];
}

static void bind_func(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    assert(e->expr == EXPR_FN);
    slice_table_put(&ctx->mod_ctx->functions, name, e);
}
static struct expr *lookup_func(struct peval_ctx *ctx, slice_t name) {
    struct expr *e = NULL;
    slice_table_get(&ctx->mod_ctx->functions, name, (void **)&e);
    assert(!e || e->expr == EXPR_FN);
    return e;
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


static void bind(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    assert(!e || e->expr == EXPR_CONST);
    slice_table_put(&ctx->symbols, name, e);
}
static struct expr *lookup(struct peval_ctx *ctx, slice_t name) {
    struct expr *e = NULL;
    slice_table_get(&ctx->symbols, name, (void **)&e);
    return e;
}

static void push_binding(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    struct peval_binding *b;
    if (ctx->binding_count == ctx->binding_capacity) {
        ctx->binding_capacity = ctx->binding_capacity ? ctx->binding_capacity * 2 : 16;
        ctx->bindings = realloc(ctx->bindings, sizeof(struct peval_binding) * ctx->binding_capacity);
    }
    b = &ctx->bindings[ctx->binding_count++];
    b->name = name;
    if (!(b->expr = lookup(ctx, name))) {
        b->expr = &missing;
    }
    bind(ctx, name, e && e->expr == EXPR_CONST ? e : NULL);
}

static void pop_bindings(struct peval_ctx *ctx, uint count) {
    uint i;
    assert(count <= ctx->binding_count);
    for (i = 0; i < count; ++i) {
        struct peval_binding *b = &ctx->bindings[ctx->binding_count - i - 1];
        if (b->expr != &missing) {
            bind(ctx, b->name, b->expr);
        }
        else {
            slice_table_remove(&ctx->symbols, b->name);
        }
    }
    ctx->binding_count -= count;
}

static void push_pending_fn(struct peval_ctx *ctx, struct expr *fn) {
    if (ctx->pending_fn_count == ctx->pending_fn_capacity) {
        ctx->pending_fn_capacity = ctx->pending_fn_capacity ? ctx->pending_fn_capacity * 2 : 16;
        ctx->pending_fns = realloc(ctx->pending_fns, sizeof(struct expr *) * ctx->pending_fn_capacity);
    }
    ctx->pending_fns[ctx->pending_fn_count++] = fn;
}

static void peval_pending_fns(struct peval_ctx *ctx, uint lowest_index_to_process) {
    ++ctx->inhibit_call_expansion;

    while (ctx->pending_fn_count > lowest_index_to_process) {
        struct expr *fn = ctx->pending_fns[--ctx->pending_fn_count];
        uint push_count = 0;
        struct expr_decl *p;

        assert(fn->expr == EXPR_FN);
        assert(fn->u.fn.body_expr);

        for (p = fn->u.fn.params; p; p = p->next) {
            push_binding(ctx, p->name, NULL);
            ++push_count;
        }

        fn->u.fn.body_expr = peval(ctx, fn->u.fn.body_expr);
        
        pop_bindings(ctx, push_count);
    }

    --ctx->inhibit_call_expansion;
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
    struct expr *e_new;
    if (!e) {
        return NULL;
    }
    ++ctx->force_full_expansion;
    e_new = peval(ctx, e);
    --ctx->force_full_expansion;
    if (e_new->expr != EXPR_CONST) {
        PEVAL_ERR(e_new, "expected constant type expression");
    }
    if (e_new->u._const.type->type != TYPE_TYPE) {
        PEVAL_ERR(e_new, "expected type expression");
    }
    return e_new;
}

static struct expr *rebind_peval(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    if (e) {
        push_name(ctx, name);
        struct expr *e_new = peval(ctx, e);
        pop_name(ctx);
        if (e_new != e && e_new->expr == EXPR_CONST) {
            bind(ctx, name, e_new);
        }
        return e_new;
    }
    return NULL;
}

static struct expr_decl *peval_fields(struct peval_ctx *ctx, struct expr_decl *f) {
    if (f) {
        struct expr_decl f_new = *f;
        f_new.type_expr = peval_type(ctx, f->type_expr);
        f_new.value_expr = rebind_peval(ctx, f->name, f->value_expr);
        check_type(ctx, f_new.value_expr, f_new.type_expr);
        f_new.next = peval_fields(ctx, f->next);
        if (f_new.type_expr != f->type_expr || f_new.value_expr != f->value_expr || f_new.next != f->next) {
            return dup_decl(ctx, &f_new);
        }
    }
    return f;
}

static struct expr_decl *peval_bindings(struct peval_ctx *ctx, struct expr_decl *b, uint *const_count) {
    if (b) {
        struct expr_decl b_new = *b;
        b_new.type_expr = peval_type(ctx, b->type_expr);
        b_new.value_expr = rebind_peval(ctx, b->name, b->value_expr);
        check_type(ctx, b_new.value_expr, b_new.type_expr);
        if (b_new.value_expr->expr == EXPR_CONST) {
            ++*const_count;
        }
        b_new.next = peval_bindings(ctx, b->next, const_count);
        if (b_new.type_expr != b->type_expr || b_new.value_expr != b->value_expr || b_new.next != b->next) {
            return dup_decl(ctx, &b_new);
        }
    }
    return b;
}

static struct expr_call_arg *peval_args(struct peval_ctx *ctx, struct expr_call_arg *a, uint *const_count) {
    if (a) {
        struct expr_call_arg a_new = *a;
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

static struct expr_call_arg *strip_const_args(struct peval_ctx *ctx, struct expr_call_arg *a) {
    if (a) {
        if (a->expr->expr == EXPR_CONST) {
            return strip_const_args(ctx, a->next);
        }
        else {
            struct expr_call_arg a_new = *a;
            a_new.next = strip_const_args(ctx, a->next);
            if (a_new.next != a->next) {
                return dup_arg(ctx, &a_new);
            }
        }
    }
    return a;
}

static struct expr_decl *strip_const_params(struct peval_ctx *ctx, struct expr_decl *p, struct expr_call_arg *a) {
    assert(a && p || !a && !p);
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

static uint hash_const_args(struct expr_decl *p, struct expr_call_arg *a, uint hash) {
    if (a) {
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
    return hash;
}

static struct expr *peval_call(struct peval_ctx *ctx, struct expr *e) {
    struct expr e_new = *e;
    int changed = 0;
    uint const_count = 0, param_count = 0;
    struct expr *fn = NULL, *fn_name_expr;
    slice_t fn_name;

    assert(e->expr == EXPR_CALL);

    fn_name_expr = e_new.u.call.fn_expr = peval(ctx, e->u.call.fn_expr);
    changed = changed || fn_name_expr != e->u.call.fn_expr;
    if (fn_name_expr->expr == EXPR_CONST) {
        if (fn_name_expr->u._const.type->type != TYPE_FN) {
            PEVAL_ERR(fn_name_expr, "expected a function");
        }
        fn_name = fn_name_expr->u._const.u.fn_name;
        fn = lookup_func(ctx, fn_name);
        if (!fn) {
            PEVAL_ERR(fn_name_expr, "function not found");
        }
        assert(fn->expr == EXPR_FN);
        param_count = decl_count(fn->u.fn.params);
    }

    e_new.u.call.args = peval_args(ctx, e->u.call.args, &const_count);
    changed = changed || e_new.u.call.args != e->u.call.args;

    if (fn && (ctx->force_full_expansion || !ctx->inhibit_call_expansion) &&
        (const_count > 0 || const_count == param_count))
    {
        uint pending_fn_count0 = ctx->pending_fn_count;
        struct expr_call_arg *a = e_new.u.call.args;
        struct expr_decl *p = fn->u.fn.params;
        for (; a; a = a->next, p = p->next) {
            check_type(ctx, a->expr, p->type_expr);
            push_binding(ctx, p->name, a->expr);
        }

        if (const_count == param_count) {
            e_new = *peval(ctx, fn->u.fn.body_expr);
            check_type(ctx, &e_new, fn->u.fn.return_type_expr);
        }
        else {
            uint hash = hash_const_args(fn->u.fn.params, e_new.u.call.args, FNV_SEED);
            uint fn_name_capacity = fn_name.len + 8;
            slice_t fn_name_new;
            struct expr *fn_new;

            fn_name_new.ptr = malloc(fn_name_capacity + 1);
            memcpy(fn_name_new.ptr, fn_name.ptr, fn_name.len);
            fn_name_new.len = fn_name.len + snprintf(fn_name_new.ptr + fn_name.len, 8, "_%u", hash);

            fn_new = lookup_func(ctx, fn_name_new);
            if (!fn_new) {
                fn_new = dup_expr(ctx, fn, fn);
                fn_new->u.fn.params = strip_const_params(ctx, fn_new->u.fn.params, e_new.u.call.args);
                bind_func(ctx, fn_name_new, fn_new);
                fn_new->u.fn.body_expr = peval(ctx, fn_new->u.fn.body_expr);
            }

            fn_name_expr = expr_create(ctx, EXPR_CONST, fn_name_expr);
            fn_name_expr->u._const.type = &type_fn;
            fn_name_expr->u._const.u.fn_name = fn_name_new;

            e_new.u.call.fn_expr = fn_name_expr;
            e_new.u.call.args = strip_const_args(ctx, e_new.u.call.args);
        }

        peval_pending_fns(ctx, pending_fn_count0);
        pop_bindings(ctx, param_count);
        changed = 1;
    }

    if (changed) {
        return dup_expr(ctx, &e_new, e);
    }
    return e;
}

struct expr *peval(struct peval_ctx *ctx, struct expr *e) {
    struct expr *result = e;
    if (!e) {
        return NULL;
    }
    switch (e->expr) {
    case EXPR_SYM: {
        struct expr *e_env = lookup(ctx, e->u.sym.name);
        if (e_env) {
            struct expr *e_new = dup_expr(ctx, e_env, e_env);
            assert(e_new->expr == EXPR_CONST);
            /* antecedent should be the symbol from the source code, not the copied constant expression */
            e_new->antecedent = e;
            result = e_new;
        }
        break;
    }
    case EXPR_STRUCT: {
        struct expr e_new = *e;
        struct expr_decl *f, *new_fields;
        uint pending_fn_count0 = ctx->pending_fn_count;
        uint push_count = 0;

        for (f = e->u._struct.fields; f; f = f->next) {
            push_binding(ctx, f->name, f->value_expr);
            ++push_count;
        }

        while (1) {
            new_fields = peval_fields(ctx, e_new.u._struct.fields);
            if (new_fields == e_new.u._struct.fields) {
                break;
            }
            e_new.u._struct.fields = new_fields;
        }

        peval_pending_fns(ctx, pending_fn_count0);
        pop_bindings(ctx, push_count);

        if (e_new.u._struct.fields != e->u._struct.fields) {
            result = dup_expr(ctx, &e_new, e);
        }
        break;
    }
    case EXPR_FN: {
        struct expr_decl *p;
        for (p = e->u.fn.params; p; p = p->next) {
            p->type_expr = peval_type(ctx, p->type_expr);
        }
        e->u.fn.return_type_expr = peval_type(ctx, e->u.fn.return_type_expr);

        if (e->u.fn.body_expr) {
            struct expr *e_new = expr_create(ctx, EXPR_CONST, e);
            e_new->u._const.type = &type_fn;
            e_new->u._const.u.fn_name = make_fn_name(ctx);
            bind_func(ctx, e_new->u._const.u.fn_name, e);
            push_pending_fn(ctx, e);
            result = e_new;
        }
        else {
            struct expr *e_new = expr_create(ctx, EXPR_CONST, e);
            e_new->u._const.type = &type_type;
            e_new->u._const.u.type = &type_fn;
            result = e_new;
        }
        break;
    }
    case EXPR_LET: {
        struct expr e_new = *e;
        int changed = 0;
        uint const_count = 0, push_count = 0;
        uint pending_fn_count0 = ctx->pending_fn_count;
        struct expr_decl *b, *new_bindings;

        for (b = e->u.let.bindings; b; b = b->next) {
            push_binding(ctx, b->name, b->value_expr);
            ++push_count;
        }
        
        while (1) {
            const_count = 0;
            new_bindings = peval_bindings(ctx, e_new.u.let.bindings, &const_count);
            if (new_bindings == e_new.u.let.bindings) {
                break;
            }
            e_new.u.let.bindings = new_bindings;
            changed = 1;
        }

        if (const_count == push_count) {
            e_new = *peval(ctx, e_new.u.let.body_expr);
            changed = 1;
        }
        else {
            e_new.u.let.body_expr = peval(ctx, e_new.u.let.body_expr);
            changed = changed || e_new.u.let.body_expr != e->u.let.body_expr;
        }

        peval_pending_fns(ctx, pending_fn_count0);
        pop_bindings(ctx, push_count);

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
    bind(ctx, name, e_type);
}

void peval_ctx_init(struct peval_ctx *ctx, struct module_ctx *mod_ctx, struct error_ctx *err_ctx) {
    memset(ctx, 0, sizeof(*ctx));

    ctx->err_ctx = err_ctx;
    ctx->mod_ctx = mod_ctx;
    ctx->closest_name.ptr = "root";
    ctx->closest_name.len = 4;

    slice_table_init(&ctx->symbols, 16);

    bind_type(ctx, "Expr", &type_expr);
    bind_type(ctx, "Type", &type_type);
    bind_type(ctx, "Unit", &type_unit);
    bind_type(ctx, "Bool", &type_bool);
    bind_type(ctx, "Int", &type_int);
}
