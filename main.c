#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mod.h"
#include "peval.h"
#include "error.h"

static void run_tests(char *filename) {
    struct global_ctx global_ctx;
    global_ctx_init(&global_ctx);

    struct module_ctx *mod_ctx = module_load(&global_ctx, NULL, slice_from_str(filename));
    if (!mod_ctx) {
        return;
    }

    printf("\nrunning tests...\n");
    struct type *t = mod_ctx->module_type;

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

        struct expr call_expr;
        memset(&call_expr, 0, sizeof(struct expr));
        call_expr.kind = EXPR_CALL;
        call_expr.call.callable_expr = e;
        printf("\nrunning test function: %s\n", func->name->data);
        fflush(stdout);
        pretty_print_indented(func->fun_expr, 1);

        struct peval_ctx peval_ctx;
        peval_ctx_init(&peval_ctx, mod_ctx);
        peval_ctx.force_full_expansion = 1;
        if (setjmp(peval_ctx.error_jmp_buf)) {
            break;
        }
        peval(&peval_ctx, &call_expr);
    }
    printf("\n%u/%u(%u) asserts passed\n", mod_ctx->asserts_hit - mod_ctx->asserts_failed, mod_ctx->asserts_hit, mod_ctx->total_assert_count);
    print_errors(&mod_ctx->err_ctx);
    module_free(mod_ctx);
    tracking_allocator_cleanup(&global_ctx.alloc);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if (argc == 3 && !strcmp(argv[1], "test")) {
        run_tests(argv[2]);
    }
    else {
        printf("usage: lang test <source_file1>+\n");
    }
    fflush(stdout);
    return 0;
}
