#ifndef SYM_H
#define SYM_H

#include "slice.h"

struct symbol {
    uint length;
    char data[0];
};

struct symbol_table {
    struct allocator *sym_alloc;
    struct slice_table table;
};

void symbol_table_init(struct symbol_table *s, struct allocator *sym_alloc, struct allocator *table_alloc);
struct symbol *intern_slice(struct symbol_table *s, slice_t name);
struct symbol *intern_string(struct symbol_table *s, char *str);

#endif
