#include "sym.h"
#include <string.h>

void symbol_table_init(struct symbol_table *s, struct allocator *sym_alloc, struct allocator *table_alloc) {
    memset(s, 0, sizeof(struct symbol_table));
    hashtable_init(SYMBOL_HASHTABLE, s->table, table_alloc, 0);
    s->sym_alloc = sym_alloc;
}

struct symbol *intern_slice(struct symbol_table *s, slice_t name) {
    struct symbol *result;
    bool found;
    hashtable_get(SYMBOL_HASHTABLE, s->table, name, result, found);
    if (!found) {
        result = allocate(s->sym_alloc, sizeof(struct symbol) + name.len + 1);
        result->length = name.len;
        memcpy(result->data, name.ptr, name.len);
        result->data[name.len] = '\0';
        hashtable_put(SYMBOL_HASHTABLE, s->table, name, result);
    }
    return result;
}

struct symbol *intern_string(struct symbol_table *s, char *str) {
    return intern_slice(s, slice_from_str(str));
}
