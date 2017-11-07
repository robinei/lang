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
        //"exp = fn(x, n: int) int: if n == 0: 1 else x * exp(x, n - 1);"
        //"main = let x = exp(2, 3) in x + 100;"
        "even = fn(n: int) bool: if n == 0: true else odd(n - 1);"
        "odd = fn(n: int) bool: if n == 0: false else even(n - 1);"
        "main = even(4);"
        ;

    struct expr *e;

    struct parse_ctx ctx = {0,};
    ctx.scan.cursor = text;
    ctx.text = text;

    if ((e = parse_module(&ctx))) {
        print_expr(e, 0);
        printf("PARSE OK\n");
        {
            struct peval_ctx peval = { 0, };
            struct expr *e_new = partial_eval_module(&peval, e);
            if (e_new) {
                print_expr(e_new, 0);
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
