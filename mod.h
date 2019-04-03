#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"
#include "slice.h"

struct function {
    struct symbol *name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct arena arena;
    struct error_ctx err_ctx;

    struct pointer_table functions;
    struct symbol_table symbol_table;

    char *source_text;
    struct expr *struct_expr;
    struct type *module_type;

    uint total_assert_count;
    uint asserts_hit;
    uint asserts_failed;
};

struct module_ctx *module_load(const char *filename);

#endif
