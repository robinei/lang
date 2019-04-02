#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parse.h"
#include "peval.h"
#include "error.h"

static void print_errors(struct error_ctx *ctx) {
    struct error_entry *entry;
    for (entry = ctx->first_error; entry; entry = entry->next) {
        error_fprint(ctx, entry, stdout);
    }
}

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

static void run_tests(char *filename) {
    struct error_ctx err_ctx;
    struct module_ctx mod_ctx;
    struct peval_ctx peval_ctx;
    struct expr *mod_struct;

    char *source_text = read_file(filename);
    if (!source_text) {
        printf("error reading file: %s\n", filename);
        return;
    }

    error_ctx_init(&err_ctx, filename, source_text);
    module_ctx_init(&mod_ctx, &err_ctx);

    mod_struct = parse_module(&mod_ctx, source_text);
    if (!mod_struct) {
        printf("error parsing test module\n");
        print_errors(&err_ctx);
        return;
    }
    printf("parsed module:\n");
    pretty_print(mod_struct);
    uint assert_count = 0;
    {
        struct expr_visit_ctx visit_ctx = {
            .visitor = count_asserts,
            .ctx = &assert_count
        };
        struct expr *e = expr_visit(&visit_ctx, mod_struct);
        assert(e == mod_struct);
    }
    mod_ctx.struct_expr = mod_struct;

    peval_ctx_init(&peval_ctx, &mod_ctx, &err_ctx);

    if (setjmp(peval_ctx.error_jmp_buf)) {
        printf("error partially evaluating test module\n");
        print_errors(&err_ctx);
        return;
    }

    peval_ctx.force_full_expansion = 1;

    struct expr *struct_expr = peval(&peval_ctx, mod_ctx.struct_expr);
    if (struct_expr->kind != EXPR_CONST || struct_expr->c.tag != &type_type || struct_expr->c.type->kind != TYPE_STRUCT) {
        printf("expected struct expr to become a type\n");
        return;
    }
    mod_ctx.module_type = struct_expr->c.type;
    struct type *t = mod_ctx.module_type;

    printf("\nrunning tests...\n");

    for (int i = 0; i < t->attrs.size; ++i) {
        if (!t->attrs.entries[i].hash) {
            continue;
        }
        struct symbol *name = t->attrs.entries[i].key;
        struct expr *e = t->attrs.entries[i].value;

        if (name->length < 4 || memcmp(name->data, "test", 4)) {
            printf("\n(skip) name doesn't start with test: %s\n", name->data);
            continue;
        }
        if (!e) {
            printf("\n(skip) missing value expression: %s\n", name->data);
            continue;
        }
        if (e->kind != EXPR_CONST) {
            printf("\n(skip) value not const: %s\n", name->data);
            continue;
        }
        if (e->c.tag->kind != TYPE_FUN) {
            printf("\n(skip) value not a function: %s\n", name->data);
            continue;
        }
        struct function *func = e->c.fun.func;
        if (func->fun_expr->fun.params) {
            printf("\n(skip) function is not zero argument: %s\n", name->data);
            continue;
        }

        struct expr call_expr = {
            .kind = EXPR_CALL,
            .call.callable_expr = e
        };
        printf("\nrunning test function: %s\n", func->name->data);
        fflush(stdout);
        pretty_print_indented(func->fun_expr, 1);

        if (setjmp(peval_ctx.error_jmp_buf)) {
            break;
        }
        peval(&peval_ctx, &call_expr);
    }
    printf("\n%u/%u(%u) asserts passed\n", peval_ctx.assert_count - peval_ctx.assert_fails, peval_ctx.assert_count, assert_count);
    print_errors(&err_ctx);
}

int main(int argc, char *argv[]) {
    if (argc == 3 && !strcmp(argv[1], "test")) {
        run_tests(argv[2]);
    }
    else {
        printf("usage: lang test <source_file1>+\n");
    }
    fflush(stdout);
    return 0;
}
