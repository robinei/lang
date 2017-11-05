#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>

#define DECL_HASH_TABLE(name, key_type, value_type)             \
    struct name##_entry {                                       \
        uint32_t hash;                                          \
        key_type key;                                           \
        value_type value;                                       \
    };                                                          \
    struct name {                                               \
        uint32_t used, size;                                    \
        struct name##_entry *entries;                           \
    };                                                          \
    void name##_clear(struct name *table);                      \
    int name##_remove(struct name *table, key_type key);        \
    int name##_find(struct name *table, key_type key, uint32_t *index_out); \
    int name##_get(struct name *table, key_type key, value_type *value_out); \
    void name##_put(struct name *table, key_type key, value_type value); \
    void name##_init(struct name *table, uint32_t initial_size); \
    void name##_free(struct name *table);

#endif
