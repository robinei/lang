#ifndef GLOBAL_H
#define GLOBAL_H

#include "expr.h"


enum scope_kind {
    SCOPE_STRUCT,
    SCOPE_FUNCTION,
    SCOPE_BLOCK
};

struct scope {
    struct scope *parent;

    struct expr_decl *decls;
    struct expr_decl **last_decl_ptr;

    struct type *self;

    enum scope_kind kind;
};

struct scope *scope_create(struct allocator *allocator, struct scope *parent, enum scope_kind kind);


#define MODULE_HASHTABLE(X) X(module_hashtable, slice_t, struct module_ctx *, slice_hash_fnv1a, slice_equals)
DECLARE_HASHTABLE(MODULE_HASHTABLE)

struct global_ctx {
    struct tracking_allocator alloc;
    struct arena_allocator arena;

    struct symbol_table symbol_table;
    struct module_hashtable modules;

    struct scope *global_scope;
};

void global_ctx_init(struct global_ctx *global_ctx);

#endif
