#include "parse.h"
#include "peval.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static char *read_file(const char *filename) {
    char *str;
    uint len;

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    str = malloc(len + 1);
    fread(str, 1, len, fp);
    str[len] = 0;

    return str;
}

static void count_asserts(struct expr_visit_ctx *ctx, struct expr *e) {
    if (e->kind == EXPR_PRIM && e->prim.kind == PRIM_ASSERT) {
        ++*(uint *)(ctx->ctx);
    }
    expr_visit_children(ctx, e);
}

struct module_ctx *module_load(slice_t filename, struct global_ctx *global_ctx) {
    struct module_ctx *mod_ctx = calloc(1, sizeof(struct module_ctx));
    mod_ctx->global_ctx = global_ctx;

    filename = slice_dup(filename, &mod_ctx->arena);

    mod_ctx->source_text = read_file(filename.ptr);
    if (!mod_ctx->source_text) {
        printf("error reading file: %s\n", filename.ptr);
        return NULL;
    }

    error_ctx_init(&mod_ctx->err_ctx, filename, slice_from_str(mod_ctx->source_text), &mod_ctx->arena);

    struct expr *mod_struct = parse_module(mod_ctx, mod_ctx->source_text);
    if (!mod_struct) {
        printf("error parsing test module\n");
        print_errors(&mod_ctx->err_ctx);
        return NULL;
    }
    printf("parsed module:\n");
    pretty_print(mod_struct);
    {
        struct expr *e = expr_run_visitor(mod_struct, count_asserts, &mod_ctx->total_assert_count, &mod_ctx->arena);
        assert(e == mod_struct);
    }
    mod_ctx->struct_expr = mod_struct;

    struct peval_ctx peval_ctx;
    peval_ctx_init(&peval_ctx, mod_ctx);
    peval_ctx.force_full_expansion = 1;

    if (setjmp(peval_ctx.error_jmp_buf)) {
        printf("error partially evaluating test module\n");
        print_errors(&mod_ctx->err_ctx);
        return NULL;
    }

    struct expr *struct_expr = peval(&peval_ctx, mod_ctx->struct_expr);
    if (struct_expr->kind != EXPR_CONST || struct_expr->c.tag != &type_type || struct_expr->c.type->kind != TYPE_STRUCT) {
        printf("expected struct expr to become a type\n");
        return NULL;
    }
    mod_ctx->module_type = struct_expr->c.type;

    return mod_ctx;
}
