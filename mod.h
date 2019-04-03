#ifndef MOD_H
#define MOD_H

#include "global.h"
#include "expr.h"
#include "error.h"
#include "slice.h"

struct function {
    struct symbol *name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct global_ctx *global_ctx;
    struct arena arena;
    struct error_ctx err_ctx;

    struct pointer_table functions;

    char *source_text;
    struct expr *struct_expr;
    struct type *module_type;

    uint total_assert_count;
    uint asserts_hit;
    uint asserts_failed;
};

struct module_ctx *module_load(slice_t filename, struct global_ctx *global_ctx);

#endif
