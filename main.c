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

static void peval_module(struct peval_ctx *ctx, struct module_ctx *mod) {
    ctx->inhibit_call_expansion = true;

    ctx->identify_closures = true;
    struct expr *temp_struct = peval(ctx, mod->struct_expr);
    ctx->identify_closures = false;
    
    while (true) {
        temp_struct = peval(ctx, mod->struct_expr);
        if (temp_struct == mod->struct_expr) {
            break;
        }
        mod->struct_expr = temp_struct;
    }
    
    ctx->inhibit_call_expansion = false;
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

    peval_module(&peval_ctx, &mod_ctx);

    printf("running tests...\n");

    peval_ctx.force_full_expansion = 1;
    decls = mod_ctx.struct_expr->u._struct.fields;
    for (; decls; decls = decls->next) {
        struct expr *e = decls->value_expr;
        struct function *func;
        slice_t fn_name;
        struct expr call_expr;
        memset(&call_expr, 0, sizeof(call_expr));
        call_expr.expr = EXPR_CALL;

        if (!e) {
            continue;
        }
        if (e->expr != EXPR_CONST) {
            continue;
        }
        if (e->u._const.type->type != TYPE_FN) {
            continue;
        }
        fn_name = e->u._const.u.fn.name;
        if (fn_name.len < 4 || memcmp(fn_name.ptr, "test", 4)) {
            continue;
        }
        if (!slice_table_get(&mod_ctx.functions, fn_name, (void **)&func)) {
            continue;
        }
        if (func->fn_expr->u.fn.params) {
            continue;
        }
        call_expr.u.call.fn_expr = e;
        printf("running test function: %.*s\n", fn_name.len, fn_name.ptr);
        fflush(stdout);

        struct print_ctx print_ctx = { 0, };
        print_expr(&print_ctx, func->fn_expr);
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
    fgetc(stdin);
    return 0;
}
