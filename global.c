#include "global.h"
#include "expr.h"
#include <string.h>

struct scope *scope_create(struct allocator *allocator, struct scope *parent, enum scope_kind kind) {
    struct scope *scope = allocate(allocator, sizeof(struct scope));
    scope->kind = kind;
    scope->last_decl_ptr = &scope->decls;
    scope->parent = parent;
    return scope;
}

static void bind_type(struct global_ctx *global_ctx, char *name_str, struct type *type) {
    struct expr *e_type = expr_create(ctx, EXPR_CONST, NULL);
    e_type->t = &type_type;
    e_type->c.type = type;

    struct expr *name_expr = expr_create(ctx, EXPR_SYM, NULL);
    name_expr->sym = intern_string(&global_ctx->symbol_table, name_str);

    struct expr_decl d;
    memset(&d, 0, sizeof(struct expr_decl));
    d.name_expr = name_expr;
    d.value_expr = e_type;

    add_single_decl_to_scope(ctx, &d);
}

void global_ctx_init(struct global_ctx *global_ctx) {
    memset(global_ctx, 0, sizeof(struct global_ctx));

    tracking_allocator_init(&global_ctx->alloc, default_allocator);
    arena_allocator_init(&global_ctx->arena, &global_ctx->alloc.a, DEFAULT_ARENA_BUFFER_SIZE);
    hashtable_init(MODULE_HASHTABLE, global_ctx->modules, &global_ctx->alloc.a, 0);
    symbol_table_init(&global_ctx->symbol_table, &global_ctx->arena.a, &global_ctx->alloc.a);

    global_ctx->global_scope = scope_create(&global_ctx->arena, NULL, SCOPE_BLOCK);

    bind_type(global_ctx, "Expr", &type_expr);
    bind_type(global_ctx, "Type", &type_type);
    bind_type(global_ctx, "Unit", &type_unit);
    bind_type(global_ctx, "Bool", &type_bool);
    bind_type(global_ctx, "Int", &type_int);
    bind_type(global_ctx, "String", &type_string);
}
