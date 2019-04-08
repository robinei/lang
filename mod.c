#include "parse.h"
#include "peval.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static slice_t read_file(const char *filename, struct allocator *a) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return (slice_t) {NULL, 0};
    }

    fseek(fp, 0, SEEK_END);
    uint len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *str = allocate(a, len + 1);
    fread(str, 1, len, fp);
    fclose(fp);
    str[len] = 0;

    return slice_from_str_len(str, len);
}

static void count_asserts(struct expr_visit_ctx *ctx, struct expr *e) {
    if (e->kind == EXPR_PRIM && e->prim.kind == PRIM_ASSERT) {
        ++*(uint *)(ctx->ctx);
    }
    expr_visit_children(ctx, e);
}

static slice_t normalize_path(struct global_ctx *global_ctx, slice_t path) {
    /* TODO: actually normalize the import path */
    return slice_dup(path, &global_ctx->arena.a);
}

struct module_ctx *module_load(struct global_ctx *global_ctx, struct module_ctx *mod_ctx, slice_t path) {
    path = normalize_path(global_ctx, path);

    struct module_ctx *new_mod_ctx;
    if (slice_table_get(&global_ctx->modules, path, (void **)&new_mod_ctx)) {
        if (!new_mod_ctx->module_type) {
            printf("circular dependency detected while importing: %.*s\n", path.len, path.ptr);
            return NULL;
        }
        return new_mod_ctx;
    }

    new_mod_ctx = allocate(&global_ctx->alloc.a, sizeof(struct module_ctx));
    tracking_allocator_init(&new_mod_ctx->alloc, &global_ctx->alloc.a);
    arena_allocator_init(&new_mod_ctx->arena, &new_mod_ctx->alloc.a, DEFAULT_ARENA_BUFFER_SIZE);
    pointer_table_init(&new_mod_ctx->functions, &new_mod_ctx->alloc.a, 0);
    new_mod_ctx->global_ctx = global_ctx;
    new_mod_ctx->source_text = read_file(path.ptr, &new_mod_ctx->arena.a);
    if (!new_mod_ctx->source_text.ptr) {
        printf("error reading file: %s\n", path.ptr);
        module_free(new_mod_ctx);
        return NULL;
    }

    error_ctx_init(&new_mod_ctx->err_ctx, path, new_mod_ctx->source_text, &new_mod_ctx->arena);

    new_mod_ctx->struct_expr = parse_module(new_mod_ctx, new_mod_ctx->source_text);
    if (!new_mod_ctx->struct_expr) {
        printf("error parsing test module\n");
        print_errors(&new_mod_ctx->err_ctx);
        module_free(new_mod_ctx);
        return NULL;
    }
    printf("parsed module:\n");
    pretty_print(new_mod_ctx->struct_expr);
    {
        struct expr *e = expr_run_visitor(new_mod_ctx->struct_expr, count_asserts, &new_mod_ctx->total_assert_count, &new_mod_ctx->arena.a);
        assert(e == new_mod_ctx->struct_expr);
    }
    new_mod_ctx->parse_mark = arena_allocator_mark(&new_mod_ctx->arena);

    struct peval_ctx peval_ctx;
    peval_ctx_init(&peval_ctx, new_mod_ctx);
    peval_ctx.force_full_expansion = 1;

    if (setjmp(peval_ctx.error_jmp_buf)) {
        printf("error partially evaluating test module\n");
        print_errors(&new_mod_ctx->err_ctx);
        module_free(new_mod_ctx);
        return NULL;
    }

    slice_table_put(&global_ctx->modules, path, new_mod_ctx);

    struct expr *struct_expr = peval(&peval_ctx, new_mod_ctx->struct_expr);
    if (struct_expr->kind != EXPR_CONST || struct_expr->c.tag != &type_type || struct_expr->c.type->kind != TYPE_STRUCT) {
        printf("expected struct expr to become a type\n");
        slice_table_remove(&global_ctx->modules, path);
        module_free(new_mod_ctx);
        return NULL;
    }
    new_mod_ctx->module_type = struct_expr->c.type;
    return new_mod_ctx;
}

void module_free(struct module_ctx *mod_ctx) {
    tracking_allocator_cleanup(&mod_ctx->alloc);
    deallocate(&mod_ctx->global_ctx->alloc.a, mod_ctx, sizeof(struct module_ctx));
}
