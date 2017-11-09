#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "peval.h"

int main(int argc, char *argv[]) {
    char *text =
        "exp = fn(x, n: Int) Int: if n == 0: 1 else x * exp(x, n - 1);"
        "exped = let x = exp(2, 3) in x + 100;"
        "mutual = let\n"
        "   even = fn(n: Int) Bool: if n == 0: true else odd(n - 1);\n"
        "   odd = fn(n: Int) Bool: if n == 0: false else even(n - 1)\n"
        "in even(5);"
        "getType = fn(i: Int) Type: if i == 0: Bool else Int;\n"
        "test = fn(): let x: getType(1) = 123 in x;\n"
        "fibHelp = fn(a, b, n: Int) Int: print(n) if n == 0: a else fibHelp(b, a+b, n-1);\n"
        "fib = fn(n: Int) Int: fibHelp(0, 1, n);\n"
        "main = fib(6);\n"
        ;

    struct expr *mod_struct;

    struct parse_ctx ctx = {0,};
    ctx.scan.cursor = text;
    ctx.text = text;

    if ((mod_struct = parse_module(&ctx))) {
        printf("PARSE OK\n");
        {
            struct peval_ctx peval = { 0, };
            struct module *mod = partial_eval_module(&peval, mod_struct);
            if (mod) {
                uint i;
                struct slice_table *funcs = &mod->functions;
                printf("\n");
                for (i = 0; i < funcs->size; ++i) {
                    if (funcs->entries[i].hash) {
                        printf("%.*s = ", funcs->entries[i].key.len, funcs->entries[i].key.ptr);
                        print_expr(NULL, funcs->entries[i].value);
                        printf(";\n");
                    }
                }
                printf("\n");
                print_expr(NULL, mod->struct_expr);
                printf("\n\n");
                printf("PARTIAL EVAL OK\n");
            }
            else {
                printf("PARTIAL EVAL ERROR\n");
            }
        }
    }
    else {
        printf("PARSE ERROR: %s\n", ctx.error);
    }

    fgetc(stdin);
    return 0;
}
