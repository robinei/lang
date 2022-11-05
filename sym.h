#ifndef SYM_H
#define SYM_H

#include "slice.h"
#include "hashtable.h"

struct symbol {
    uint length;
    char data[0];
};

#define SYMBOL_HASHTABLE(X) X(symbol_hashtable, slice_t, struct symbol *, slice_hash_fnv1a, slice_equals)
DECLARE_HASHTABLE(SYMBOL_HASHTABLE)

struct symbol_table {
    struct allocator *sym_alloc;
    struct symbol_hashtable table;
};

void symbol_table_init(struct symbol_table *s, struct allocator *sym_alloc, struct allocator *table_alloc);
struct symbol *intern_slice(struct symbol_table *s, slice_t name);
struct symbol *intern_string(struct symbol_table *s, char *str);

#endif
