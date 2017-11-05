#include "peval.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct expr *peval(struct peval_ctx *ctx, struct expr *e);
static struct expr *eval_prim(struct peval_ctx *ctx, struct expr *e);

struct peval_binding {
    slice_t name;
    struct expr *expr;
};

static struct expr missing;

static struct expr *expr_create(struct peval_ctx *ctx, uint expr_type) {
    struct expr *e = calloc(1, sizeof(struct expr));
    e->expr = expr_type;
    return e;
}

static struct expr *dup_expr(struct peval_ctx *ctx, struct expr *e) {
    struct expr *e_copy = calloc(1, sizeof(struct expr));
    *e_copy = *e;
    return e_copy;
}


static void peval_error(struct peval_ctx *ctx, const char *error) {
    printf("error during partial eval: %s\n", error);
    longjmp(ctx->error_jmp_buf, 1);
}

static struct expr *lookup(struct peval_ctx *ctx, slice_t name) {
    struct expr *e = NULL;
    slice_table_get(&ctx->symbols, name, &e);
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
    if (!slice_table_get(&ctx->symbols, name, &b->expr)) {
        b->expr = &missing;
    }
    slice_table_put(&ctx->symbols, name, e);
}

static void pop_bindings(struct peval_ctx *ctx, uint count) {
    uint i;
    assert(count <= ctx->binding_count);
    for (i = 0; i < count; ++i) {
        struct peval_binding *b = &ctx->bindings[ctx->binding_count - i - 1];
        if (b->expr != &missing) {
            slice_table_put(&ctx->symbols, b->name, b->expr);
        }
        else {
            slice_table_remove(&ctx->symbols, b->name);
        }
    }
    ctx->binding_count -= count;
}

static struct expr *rebind_peval(struct peval_ctx *ctx, slice_t name, struct expr *e) {
    if (e) {
        struct expr *e_new = peval(ctx, e);
        if (e_new != e) {
            slice_table_put(&ctx->symbols, name, e_new);
        }
        return e_new;
    }
    return NULL;
}

static void check_type(struct peval_ctx *ctx, struct expr *e, struct expr *e_type) {
    if (!e_type || e->expr != EXPR_CONST || e_type->expr != EXPR_CONST) {
        return;
    }
    assert(e_type->u._const.type->type == TYPE_TYPE);
    if (e_type->u._const.u.type != e->u._const.type) {
        peval_error(ctx, "type mismatch");
    }
}

static struct expr *peval(struct peval_ctx *ctx, struct expr *e) {
    if (!e) {
        return NULL;
    }
    switch (e->expr) {
    case EXPR_SYM: {
        struct expr *e_new = lookup(ctx, e->u.sym.name);
        if (e_new && e_new->expr == EXPR_CONST) {
            return e_new;
        }
        break;
    }
    case EXPR_STRUCT: {
        struct expr e_new = *e;
        int changed = 0;
        uint pushed = 0;
        struct expr_struct_field *f, *f_new, **slot;

        for (f = e->u._struct.fields; f; f = f->next) {
            push_binding(ctx, f->name, f->value_expr);
            ++pushed;
        }

        slot = &e_new.u._struct.fields;
        for (f = *slot; f; f = f->next) {
            *slot = f_new = calloc(1, sizeof(struct expr_struct_field));
            *f_new = *f;
            f_new->value_expr = rebind_peval(ctx, f->name, f->value_expr);
            check_type(ctx, f_new->value_expr, f_new->type_expr);
            changed = changed || f_new->value_expr != f->value_expr;
            slot = &f_new->next;
        }

        assert(pushed == e->u._struct.field_count);
        pop_bindings(ctx, e->u._struct.field_count);

        if (changed) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    case EXPR_FN: {
        struct expr e_new = *e;
        e_new.u.fn.body_expr = peval(ctx, e->u.fn.body_expr);
        check_type(ctx, e_new.u.fn.body_expr, e_new.u.fn.return_type_expr);
        if (e_new.u.fn.body_expr != e->u.fn.body_expr) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    case EXPR_LET: {
        struct expr e_new = *e;
        int changed = 0;
        uint pushed = 0;
        uint const_count = 0;
        struct expr_let_binding *b, *b_new, **slot;

        for (b = e->u.let.bindings; b; b = b->next) {
            push_binding(ctx, b->name, b->value_expr);
            ++pushed;
        }
        
        slot = &e_new.u.let.bindings;
        for (b = *slot; b; b = b->next) {
            *slot = b_new = calloc(1, sizeof(struct expr_let_binding));
            *b_new = *b;
            b_new->value_expr = rebind_peval(ctx, b->name, b->value_expr);
            check_type(ctx, b_new->value_expr, b_new->type_expr);
            changed = changed || b_new->value_expr != b->value_expr;
            if (b_new->value_expr->expr == EXPR_CONST) {
                ++const_count;
            }
            slot = &b_new->next;
        }

        if (const_count == e_new.u.let.binding_count) {
            e_new = *peval(ctx, e->u.let.body_expr);
            changed = 1;
        }
        else {
            e_new.u.let.body_expr = peval(ctx, e->u.let.body_expr);
            changed = changed || e_new.u.let.body_expr != e->u.let.body_expr;
        }

        assert(pushed == e->u.let.binding_count);
        pop_bindings(ctx, e->u.let.binding_count);

        if (changed) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    case EXPR_PRIM: {
        struct expr e_new = *e;
        int changed = 0;
        uint const_count = 0;
        struct expr_prim_arg *a, *a_new, **slot;

        slot = &e_new.u.prim.args;
        for (a = *slot; a; a = a->next) {
            *slot = a_new = calloc(1, sizeof(struct expr_prim_arg));
            *a_new = *a;
            a_new->expr = peval(ctx, a->expr);
            changed = changed || a_new->expr != a->expr;
            if (a_new->expr->expr == EXPR_CONST) {
                ++const_count;
            }
            slot = &a_new->next;
        }

        if (const_count == e->u.prim.arg_count) {
            return eval_prim(ctx, &e_new);
        }
        if (changed) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    case EXPR_CALL: {
        struct expr e_new = *e;
        int changed = 0;
        uint pushed = 0;
        uint const_count = 0;
        struct expr_call_arg *a, *a_new, **slot;
        struct expr *fn;

        fn = e_new.u.call.fn_expr = peval(ctx, e->u.call.fn_expr);
        changed = changed || fn != e->u.call.fn_expr;
        if (fn && fn->expr == EXPR_SYM) {
            fn = lookup(ctx, fn->u.sym.name);
        }

        slot = &e_new.u.call.args;
        for (a = *slot; a; a = a->next) {
            *slot = a_new = calloc(1, sizeof(struct expr_call_arg));
            *a_new = *a;
            a_new->expr = peval(ctx, a->expr);
            changed = changed || a_new->expr != a->expr;
            if (a_new->expr->expr == EXPR_CONST) {
                ++const_count;
            }
            slot = &a_new->next;
        }

        if (fn && fn->expr == EXPR_FN && (const_count > 0 || const_count == fn->u.fn.param_count)) {
            struct expr_fn_param *p = fn->u.fn.params;
            for (a = e_new.u.call.args; a; a = a->next, p = p->next) {
                if (a->expr->expr == EXPR_CONST) {
                    check_type(ctx, a->expr, p->type_expr);
                    push_binding(ctx, p->name, a->expr);
                    ++pushed;
                }
            }

            if (const_count == fn->u.fn.param_count) {
                e_new = *peval(ctx, fn->u.fn.body_expr);
                check_type(ctx, &e_new, fn->u.fn.return_type_expr);
                changed = 1;
            }
            else {
                peval_error(ctx, "unimplemented partial eval of call");
            }

            assert(pushed == const_count);
            pop_bindings(ctx, const_count);
        }

        if (changed) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    case EXPR_IF: {
        struct expr e_new = *e;
        int changed = 0;

        e_new.u._if.cond_expr = peval(ctx, e->u._if.cond_expr);
        changed = changed || e_new.u._if.cond_expr != e->u._if.cond_expr;

        if (e_new.u._if.cond_expr->expr == EXPR_CONST) {
            struct expr_const *_const = &e_new.u._if.cond_expr->u._const;
            if (_const->type->type != TYPE_BOOL) {
                peval_error(ctx, "if conditional must be boolean");
            }
            e_new = *peval(ctx, _const->u._bool ? e->u._if.then_expr : e->u._if.else_expr);
            changed = 1;
        }
        else {
            e_new.u._if.then_expr = peval(ctx, e->u._if.then_expr);
            e_new.u._if.else_expr = peval(ctx, e->u._if.else_expr);
            if (e_new.u._if.then_expr->expr == EXPR_CONST && e_new.u._if.else_expr->expr == EXPR_CONST &&
                (e_new.u._if.then_expr->u._const.type != e_new.u._if.else_expr->u._const.type)) {
                peval_error(ctx, "mismatching types in if arms");
            }
            changed = changed || e_new.u._if.then_expr != e->u._if.then_expr;
            changed = changed || e_new.u._if.else_expr != e->u._if.else_expr;
        }

        if (changed) {
            return dup_expr(ctx, &e_new);
        }
        break;
    }
    }
    return e;
}

static struct expr *peval_type(struct peval_ctx *ctx, struct expr *e) {
    struct expr *e_new;
    if (!e) {
        return NULL;
    }
    e_new = peval(ctx, e);
    if (e_new->expr != EXPR_CONST) {
        peval_error(ctx, "expected constant type expression");
    }
    if (e_new->u._const.type->type != TYPE_TYPE) {
        peval_error(ctx, "expected type expression");
    }
    return e_new;
}

static void peval_type_exprs(struct peval_ctx *ctx, struct expr *e) {
    if (!e) {
        return;
    }
    switch (e->expr) {
    case EXPR_STRUCT: {
        struct expr_struct_field *f;
        for (f = e->u._struct.fields; f; f = f->next) {
            f->type_expr = peval_type(ctx, f->type_expr);
            peval_type_exprs(ctx, f->value_expr);
        }
        break;
    }
    case EXPR_FN: {
        struct expr_fn_param *p;
        for (p = e->u.fn.params; p; p = p->next) {
            p->type_expr = peval_type(ctx, p->type_expr);
        }
        e->u.fn.return_type_expr = peval_type(ctx, e->u.fn.return_type_expr);
        peval_type_exprs(ctx, e->u.fn.body_expr);
        break;
    }
    case EXPR_LET: {
        struct expr_let_binding *b;
        for (b = e->u.let.bindings; b; b = b->next) {
            b->type_expr = peval_type(ctx, b->type_expr);
            peval_type_exprs(ctx, b->value_expr);
        }
        peval_type_exprs(ctx, e->u.let.body_expr);
        break;
    }
    case EXPR_PRIM: {
        struct expr_prim_arg *a;
        for (a = e->u.prim.args; a; a = a->next) {
            peval_type_exprs(ctx, a->expr);
        }
        break;
    }
    case EXPR_CALL: {
        struct expr_call_arg *a;
        for (a = e->u.call.args; a; a = a->next) {
            peval_type_exprs(ctx, a->expr);
        }
        break;
    }
    case EXPR_IF:
        peval_type_exprs(ctx, e->u._if.cond_expr);
        peval_type_exprs(ctx, e->u._if.then_expr);
        peval_type_exprs(ctx, e->u._if.else_expr);
        break;
    }
}

static void bind_type(struct peval_ctx *ctx, char *name_str, struct type *type) {
    struct expr *e_type;
    slice_t name;

    name.ptr = name_str;
    name.len = strlen(name_str);
    e_type = expr_create(ctx, EXPR_CONST);
    e_type->u._const.type = &type_type;
    e_type->u._const.u.type = type;
    slice_table_put(&ctx->symbols, name, e_type);
}

struct expr *partial_eval_module(struct peval_ctx *ctx, struct expr *e) {
    struct expr *e_new;
    struct expr_struct_field *f;

    assert(e->expr == EXPR_STRUCT);

    if (setjmp(ctx->error_jmp_buf)) {
        return NULL;
    }

    slice_table_init(&ctx->symbols, 16);
    slice_table_init(&ctx->functions, 16);

    bind_type(ctx, "type", &type_type);
    bind_type(ctx, "bool", &type_bool);
    bind_type(ctx, "int", &type_int);

    for (f = e->u._struct.fields; f; f = f->next) {
        push_binding(ctx, f->name, f->value_expr);
    }
    peval_type_exprs(ctx, e);
    pop_bindings(ctx, e->u._struct.field_count);
    
    while (1) {
        e_new = peval(ctx, e);
        if (e_new == e) {
            break;
        }
        e = e_new;
    }

    return e_new;
}

static int int_value(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_CONST);
    if (e->u._const.type->type != TYPE_INT) {
        peval_error(ctx, "expected int");
    }
    return e->u._const.u._int;
}

static int const_eq(struct peval_ctx *ctx, struct expr *a, struct expr *b) {
    assert(a->expr == EXPR_CONST && b->expr == EXPR_CONST);
    if (a->u._const.type != b->u._const.type) {
        return 0;
    }
    switch (a->u._const.type->type) {
    case TYPE_TYPE:
        return a->u._const.u.type == b->u._const.u.type;
    case TYPE_BOOL:
        return a->u._const.u._bool == b->u._const.u._bool;
    case TYPE_INT:
        return a->u._const.u._int == b->u._const.u._int;
    default:
        peval_error(ctx, "equality not implemented for type");
        return 0;
    }
}

#define EQUALS() const_eq(ctx, e->u.prim.args->expr, e->u.prim.args->next->expr)

#define BINOP(op, value_getter) \
    value_getter(ctx, e->u.prim.args->expr) op value_getter(ctx, e->u.prim.args->next->expr)

#define RETURN_CONST(type_ptr, value_field, value_expression) \
    { \
        struct expr *e_new = expr_create(ctx, EXPR_CONST); \
        e_new->u._const.type = type_ptr; \
        e_new->u._const.u.value_field = value_expression; \
        return e_new; \
    }

static struct expr *eval_prim(struct peval_ctx *ctx, struct expr *e) {
    assert(e->expr == EXPR_PRIM);

    switch (e->u.prim.prim) {
    case PRIM_EQ: RETURN_CONST(&type_bool, _bool, EQUALS())
    case PRIM_NEQ: RETURN_CONST(&type_bool, _bool, !EQUALS())
    case PRIM_ADD: RETURN_CONST(&type_int, _int, BINOP(+, int_value))
    case PRIM_SUB: RETURN_CONST(&type_int, _int, BINOP(-, int_value))
    case PRIM_MUL: RETURN_CONST(&type_int, _int, BINOP(*, int_value))
    case PRIM_DIV: RETURN_CONST(&type_int, _int, BINOP(/, int_value))
    default:
        peval_error(ctx, "invalid primitive");
        return NULL;
    }
}
