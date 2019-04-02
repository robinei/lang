#ifndef MOD_H
#define MOD_H

#include "expr.h"
#include "error.h"
#include "slice.h"

#define EXPAND_INTERFACE
#define NAME        pointer_table
#define KEY_TYPE    void *
#define VALUE_TYPE  void *
#include "hashtable.h"

struct symbol {
    uint length;
    char data[0];
};

struct function {
    struct symbol *name;
    struct expr *fun_expr;
};

struct module_ctx {
    struct error_ctx *err_ctx;
    struct pointer_table functions;
    struct slice_table symbol_table;
    struct expr *struct_expr;
    struct type *module_type;
};

void module_ctx_init(struct module_ctx *ctx, struct error_ctx *err_ctx);
struct symbol *intern_slice(struct module_ctx *ctx, slice_t name);
struct symbol *intern_string(struct module_ctx *ctx, char *str);

#endif
