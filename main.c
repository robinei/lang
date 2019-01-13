#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static void run_tests(char *filename) {
    struct error_ctx err_ctx;
    struct module_ctx mod_ctx;
    struct peval_ctx peval_ctx;
    struct expr *mod_struct;
    struct expr_decl *decls;

    char *source_text = read_file(filename);
    if (!source_text) {
        printf("error reading file: %s\n", filename);
        return;
    }

    error_ctx_init(&err_ctx, filename, source_text);

    mod_struct = parse_module(source_text, &err_ctx);
    if (!mod_struct) {
        printf("error parsing test module\n");
        print_errors(&err_ctx);
        return;
    }

    module_ctx_init(&mod_ctx, mod_struct);

    peval_ctx_init(&peval_ctx, &mod_ctx, &err_ctx);

    if (setjmp(peval_ctx.error_jmp_buf)) {
        printf("error partially evaluating test module\n");
        print_errors(&err_ctx);
        return;
    }

    mod_ctx.struct_expr = peval(&peval_ctx, mod_ctx.struct_expr);

    printf("running tests...\n");

    peval_ctx.force_full_expansion = 1;
    decls = mod_ctx.struct_expr->struc.fields;
    for (; decls; decls = decls->next) {
        struct expr *e = decls->value_expr;
        struct function *func;
        struct expr call_expr;
        memset(&call_expr, 0, sizeof(call_expr));
        call_expr.expr_kind = EXPR_CALL;

        if (!e) {
            continue;
        }
        if (e->expr_kind != EXPR_CONST) {
            continue;
        }
        if (e->c.tag->type_kind != TYPE_FUN) {
            continue;
        }
        func = e->c.fun.func;
        if (func->name.len < 4 || memcmp(func->name.ptr, "test", 4)) {
            continue;
        }
        if (func->fun_expr->fun.params) {
            continue;
        }
        call_expr.call.callable_expr = e;
        printf("running test function: %.*s\n", func->name.len, func->name.ptr);
        fflush(stdout);

        struct print_ctx print_ctx = { 0, };
        print_expr(&print_ctx, func->fun_expr);
        printf("\n\n");

        if (setjmp(peval_ctx.error_jmp_buf)) {
            break;
        }
        peval(&peval_ctx, &call_expr);
    }
    printf("%u/%u asserts passed\n", peval_ctx.assert_count - peval_ctx.assert_fails, peval_ctx.assert_count);
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
