#include "sym.h"
#include <string.h>

void symbol_table_init(struct symbol_table *s, struct arena *arena) {
    memset(s, 0, sizeof(struct symbol_table));
    s->arena = arena;
}

struct symbol *intern_slice(struct symbol_table *s, slice_t name) {
    struct symbol *result;
    if (!slice_table_get(&s->table, name, (void **)&result)) {
        result = arena_alloc(s->arena, sizeof(struct symbol) + name.len + 1);
        result->length = name.len;
        memcpy(result->data, name.ptr, name.len);
        result->data[name.len] = '\0';
        slice_table_put(&s->table, name, result);
    }
    return result;
}

struct symbol *intern_string(struct symbol_table *s, char *str) {
    return intern_slice(s, slice_from_str(str));
}
