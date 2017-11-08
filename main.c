#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "peval.h"

int main(int argc, char *argv[]) {
    char *text =
        /*"StructTest = struct a, b: int; c = 1;;\n"
        "FnType = fn(x, y: int) int;\n"
        "Foo = 1 + 2 3;\n"
        "Bar = let x = 1; y = 2 in x;\n"
        "Baz = fn(a, b: int; c) int: a + b * c;\n"
        "Bax = fn(x): x;\n"
        "Bax = fn(x; y: int): x;\n"
        "IfTest = if 1: 2 3 elif 4 5: 6 else 7;\n"
        "IfTest2 = if 1 == 2: 3 end 1;\n"
        "CallTest = Baz(1, 2, 3)();\n"
        "LeftAssoc = 1 * 2 * 3;\n"*/
        /*"exp = fn(x, n: int) int: if n == 0: 1 else x * exp(x, n - 1);"
        "main = let x = exp(2, 3) in x + 100;"
        "main2 = let\n"
        "   even = fn(n: int) bool: if n == 0: true else odd(n - 1);\n"
        "   odd = fn(n: int) bool: if n == 0: false else even(n - 1)\n"
        "in even(5);"*/
        "fib_help = fn(a, b, n: int) int: print(n) if n == 0: a else fib_help(b, a+b, n-1);\n"
        "fib = fn(n: int) int: fib_help(0, 1, n);\n"
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
                for (i = 0; i < funcs->size; ++i) {
                    if (funcs->entries[i].hash) {
                        printf("%.*s: ", funcs->entries[i].key.len, funcs->entries[i].key.ptr);
                        //print_expr(funcs->entries[i].value, 0);
                    }
                }
                print_expr(mod->struct_expr, 0);
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
