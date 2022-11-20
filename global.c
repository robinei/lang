#include "global.h"
#include "mod.h"
#include "expr.h"
#include <string.h>

static bool is_const_fun(struct expr *e) {
    return e && e->kind == EXPR_CONST && e->t->kind == TYPE_FUN;
}

static struct scope *scope_create_root(struct allocator *alloc) {
    struct scope *scope = allocate(alloc, sizeof(struct scope));
    scope->parent = NULL;
    hashtable_init(SCOPE_HASHTABLE, scope->table, free_capable_allocator(alloc), 0);
    return scope;
}

struct scope *scope_create(struct allocator *alloc, struct scope *parent) {
    assert(parent);
    struct scope *scope = allocate(alloc, sizeof(struct scope));
    scope->parent = parent;
    hashtable_init(SCOPE_HASHTABLE, scope->table, free_capable_allocator(alloc), 0);
    return scope;
}

void scope_define(struct scope *scope, struct symbol *sym, struct expr *e) {
    hashtable_put(SCOPE_HASHTABLE, scope->table, sym, e);
    if (is_const_fun(e) && !e->c.fun->name) {
        e->c.fun->name = sym;
    }
}

struct expr *scope_lookup(struct scope *scope, struct symbol *sym) {
    for (; scope; scope = scope->parent) {
        bool found;
        struct expr *e;
        hashtable_get(SCOPE_HASHTABLE, scope->table, sym, e, found);
        if (found) {
            return e;
        }
        if (scope->self) {
            e = type_get_attr(scope->self, sym);
            if (e) {
                return e;
            }
        }
    }
    return NULL;
}

static void bind_type(struct global_ctx *global_ctx, char *name_str, struct type *type) {
    struct symbol *sym = intern_string(&global_ctx->symbol_table, name_str);
    
    struct expr *e = allocate(&global_ctx->arena.a, sizeof(struct expr));
    e->kind = EXPR_CONST;
    e->c.type = type;
    e->t = &type_type;

    scope_define(global_ctx->global_scope, sym, e);
}

void global_ctx_init(struct global_ctx *global_ctx) {
    memset(global_ctx, 0, sizeof(struct global_ctx));

    tracking_allocator_init(&global_ctx->alloc, default_allocator);
    arena_allocator_init(&global_ctx->arena, &global_ctx->alloc.a, DEFAULT_ARENA_BUFFER_SIZE);
    hashtable_init(MODULE_HASHTABLE, global_ctx->modules, &global_ctx->alloc.a, 0);
    symbol_table_init(&global_ctx->symbol_table, &global_ctx->arena.a, &global_ctx->alloc.a);

    global_ctx->global_scope = scope_create_root(&global_ctx->arena.a);

    bind_type(global_ctx, "Expr", &type_expr);
    bind_type(global_ctx, "Type", &type_type);
    bind_type(global_ctx, "Unit", &type_unit);
    bind_type(global_ctx, "Bool", &type_bool);
    bind_type(global_ctx, "Int", &type_int);
    bind_type(global_ctx, "String", &type_string);
}
