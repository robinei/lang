#ifndef GLOBAL_H
#define GLOBAL_H

#include "expr.h"


#define SCOPE_HASHTABLE(X) X(scope_hashtable, struct symbol *, struct expr *, calc_ptr_hash, VALUE_EQ)
DECLARE_HASHTABLE(SCOPE_HASHTABLE)

enum scope_kind {
    SCOPE_STRUCT,
    SCOPE_FUNCTION,
    SCOPE_BLOCK
};

struct scope {
    enum scope_kind kind;
    struct scope *parent;
    struct type *self;
    struct scope_hashtable table;
};

struct scope *scope_create(struct allocator *alloc, struct scope *parent, enum scope_kind kind);
void scope_define(struct scope *scope, struct symbol *sym, struct expr *e);
struct expr *scope_lookup(struct scope *scope, struct symbol *sym);


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
