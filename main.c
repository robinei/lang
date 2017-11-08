#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "peval.h"

int main(int argc, char *argv[]) {
    char *text =
        /*"StructTest = struct a, b: Int; c = 1;;\n"
        "FnType = fn(x, y: Int) Int;\n"
        "Foo = 1 + 2 3;\n"
        "Bar = let x = 1; y = 2 in x;\n"
        "Baz = fn(a, b: Int; c) Int: a + b * c;\n"
        "Bax = fn(x): x;\n"
        "Bax = fn(x; y: Int): x;\n"
        "IfTest = if 1: 2 3 elif 4 5: 6 else 7;\n"
        "IfTest2 = if 1 == 2: 3 end 1;\n"
        "CallTest = Baz(1, 2, 3)();\n"
        "LeftAssoc = 1 * 2 * 3;\n"*/
        /*"exp = fn(x, n: Int) Int: if n == 0: 1 else x * exp(x, n - 1);"
        "main = let x = exp(2, 3) in x + 100;"
        "main2 = let\n"
        "   even = fn(n: Int) Bool: if n == 0: true else odd(n - 1);\n"
        "   odd = fn(n: Int) Bool: if n == 0: false else even(n - 1)\n"
        "in even(5);"*/
        "even = fn(n: Int) Bool: if n == 0: true else odd(n - 1);\n"
        "odd = fn(n: Int) Bool: if n == 0: false else even(n - 1);\n"
        "fib_help = fn(a, b, n: Int) Int: print(even(n)) if n == 0: a else fib_help(b, a+b, n-1);\n"
        "fib = fn(n: Int) Int: fib_help(0, 1, n);\n"
        "main = fib(6);\n"
        ;

    struct expr *mod_struct;

    struct parse_ctx ctx = {0,};
    ctx.scan.cursor = text;
    ctx.text = text;

    if ((mod_struct = parse_module(&ctx))) {
        //print_expr(e, 0);
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
