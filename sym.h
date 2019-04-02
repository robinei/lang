#ifndef SYM_H
#define SYM_H

#include "arena.h"
#include "slice.h"

struct symbol {
    uint length;
    char data[0];
};

struct symbol_table {
    struct arena *arena;
    struct slice_table table;
};

void symbol_table_init(struct symbol_table *s, struct arena *arena);
struct symbol *intern_slice(struct symbol_table *s, slice_t name);
struct symbol *intern_string(struct symbol_table *s, char *str);

#endif
