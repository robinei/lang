#include <stdio.h>
#include <stdlib.h>
#include "parse.h"

int main(int argc, char *argv[]) {
    char *text =
        "module Test;\n"
        "import IO, String;\n"

        "Foo = 1 + 2 3;\n"
        "Bar = let x = 1 in x;\n"
        "Baz = fn(a, b: int; c: int) int: a + b * c;\n"
        "IfTest = if 1: 2 3 elif 4 5: 6 else 7;;\n"
        "CallTest = Baz(1, 2, 3)();"
        ;

    struct parse_ctx ctx = {0,};
    ctx.scan.cursor = text;
    ctx.text = text;

    if (parse_module(&ctx)) {
        printf("OK\n");
    }
    else {
        printf("ERROR: %s\n", ctx.error);
    }

    fgetc(stdin);
    return 0;
}