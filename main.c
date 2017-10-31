#include <stdio.h>
#include <stdlib.h>
#include "parse.h"

int main(int argc, char *argv[]) {
    char *text =
        "StructTest = struct a, b: int; c = 1;;\n"
        "FnType = fn(x, y: int) int;\n"
        "Foo = 1 + 2 3;\n"
        "Bar = let x = 1; y = 2 in x;\n"
        "Baz = fn(a, b: int; c) int: a + b * c;\n"
        "Bax = fn(x): x;\n"
        "Bax = fn(x; y: int): x;\n"
        "IfTest = if 1: 2 3 elif 4 5: 6 else 7;\n"
        "IfTest2 = if 1 == 2: 3 end 1;\n"
        "CallTest = Baz(1, 2, 3)();\n"
        "LeftAssoc = 1 * 2 * 3;\n"
        ;

    struct expr *e;

    struct parse_ctx ctx = {0,};
    ctx.scan.cursor = text;
    ctx.text = text;

    if ((e = parse_module(&ctx))) {
        print_expr(e, 0);
        printf("OK\n");
    }
    else {
        printf("ERROR: %s\n", ctx.error);
    }

    fgetc(stdin);
    return 0;
}