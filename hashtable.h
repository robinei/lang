#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <string.h>
#include <assert.h>
#include "murmur3.h"

static uint32_t hashutil_str_hash(const char *key) {
    uint32_t hash;
    MurmurHash3_x86_32(key, strlen(key), 0, &hash);
    return hash;
}
static int hashutil_str_equals(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

static uint32_t hashutil_ptr_hash(void *ptr) {
    uint32_t val = (uint32_t)(intptr_t)ptr;
    val = ~val + (val << 15);
    val = val ^ (val >> 12);
    val = val + (val << 2);
    val = val ^ (val >> 4);
    val = val * 2057;
    val = val ^ (val >> 16);
    return val;
}
static int hashutil_ptr_equals(void *a, void *b) {
    return a == b;
}

static uint32_t hashutil_next_pow2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static uint32_t hashutil_dist_to_start(uint32_t table_size, uint32_t hash, uint32_t index_stored) {
    assert(hash);
    uint32_t start_index = hash & (table_size - 1);
    if (start_index <= index_stored) {
        return index_stored - start_index;
    }
    return index_stored + (table_size - start_index);
}

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
    int name##_get(struct name *table, key_type key, value_type *value_out); \
    void name##_put(struct name *table, key_type key, value_type value); \
    void name##_init(struct name *table, uint32_t initial_size); \
    void name##_free(struct name *table);

#define IMPL_HASH_TABLE(name, key_type, value_type, key_hasher, key_equals) \
    void name##_clear(struct name *table) {                             \
        for (uint32_t i = 0; i < table->size; ++i) {                    \
            table->entries[i].hash = 0;                                 \
        }                                                               \
        table->used = 0;                                                \
    }                                                                   \
    uint32_t name##_calc_hash(key_type key) {                           \
        uint32_t hash = key_hasher(key);                                \
        return hash ? hash : 1;                                         \
    }                                                                   \
    static int name##_find(struct name *table, key_type key, uint32_t *index_out) { \
        if (table->used == 0) {                                         \
            return 0;                                                   \
        }                                                               \
        uint32_t hash = name##_calc_hash(key);                          \
        uint32_t start_index = hash & (table->size - 1);                \
        for (uint32_t i = 0; i < table->size; ++i) {                    \
            uint32_t index = (start_index + i) & (table->size - 1);     \
            struct name##_entry *slot = table->entries + index;         \
            if (slot->hash == hash && key_equals(slot->key, key)) {     \
                *index_out = index;                                     \
                return 1;                                               \
            }                                                           \
            if (slot->hash == 0) {                                      \
                break;                                                  \
            }                                                           \
            uint32_t d = hashutil_dist_to_start(table->size, slot->hash, index); \
            if (i > d) {                                                \
                break;                                                  \
            }                                                           \
        }                                                               \
        return 0;                                                       \
    }                                                                   \
    static void name##_put_entry(struct name *table, struct name##_entry entry) { \
        uint32_t start_index = entry.hash & (table->size - 1);          \
        uint32_t probe = 0;                                             \
        for (uint32_t i = 0; i < table->size; ++i, ++probe) {           \
            uint32_t index = (start_index + i) & (table->size - 1);     \
            struct name##_entry *slot = table->entries + index;         \
            if (slot->hash == 0) {                                      \
                ++table->used;                                          \
                *slot = entry;                                          \
                return;                                                 \
            }                                                           \
            if (slot->hash == entry.hash && key_equals(slot->key, entry.key)) { \
                slot->value = entry.value;                              \
                return;                                                 \
            }                                                           \
            uint32_t slot_probe = hashutil_dist_to_start(table->size, slot->hash, index); \
            if (probe > slot_probe) {                                   \
                struct name##_entry temp = entry;                       \
                entry = *slot;                                          \
                *slot = temp;                                           \
                probe = slot_probe;                                     \
            }                                                           \
        }                                                               \
    }                                                                   \
    int name##_remove(struct name *table, key_type key) {               \
        struct name##_entry temp;                                       \
        uint32_t index;                                                 \
        if (!name##_find(table, key, &index)) {                         \
            return 0;                                                   \
        }                                                               \
        for (uint32_t i = 0; i < table->size; ++i) {                    \
            uint32_t curr_index = (index + i) & (table->size - 1);      \
            uint32_t next_index = (index + i + 1) & (table->size - 1);  \
            uint32_t next_hash = table->entries[next_index].hash;       \
            if (next_hash == 0 || hashutil_dist_to_start(table->size, next_hash, next_index) == 0) { \
                table->entries[curr_index].hash = 0;                    \
                --table->used;                                          \
                return 1;                                               \
            }                                                           \
            temp = table->entries[curr_index];                          \
            table->entries[curr_index] = table->entries[next_index];    \
            table->entries[next_index] = temp;                          \
        }                                                               \
        assert(0 && "control flow should not get here");                \
        return 0;                                                       \
    }                                                                   \
    static void name##_resize(struct name *table, uint32_t new_size) {  \
        uint32_t old_used = table->used;                                \
        uint32_t old_size = table->size;                                \
        struct name##_entry *old_entries = table->entries;              \
        table->used = 0;                                                \
        table->size = new_size;                                         \
        table->entries = (struct name##_entry *)calloc(1, sizeof(struct name##_entry) * new_size); \
        if (old_used) {                                                 \
            assert(old_used <= new_size);                               \
            for (uint32_t i = 0; i < old_size; ++i) {                   \
                struct name##_entry *slot = old_entries + i;            \
                if (slot->hash) {                                       \
                    name##_put_entry(table, *slot);                     \
                }                                                       \
            }                                                           \
            free(old_entries);                                          \
        }                                                               \
    }                                                                   \
    int name##_get(struct name *table, key_type key, value_type *value_out) { \
        uint32_t index;                                                 \
        if (name##_find(table, key, &index)) {                          \
            *value_out = table->entries[index].value;                   \
            return 1;                                                   \
        }                                                               \
        return 0;                                                       \
    }                                                                   \
    void name##_put(struct name *table, key_type key, value_type value) { \
        struct name##_entry entry;                                      \
        if (!table->size || (float)table->used / table->size > 0.85f) { \
            name##_resize(table, table->size ? table->size * 2 : 16);   \
        }                                                               \
        entry.hash = name##_calc_hash(key);                             \
        entry.key = key;                                                \
        entry.value = value;                                            \
        name##_put_entry(table, entry);                                 \
    }                                                                   \
    void name##_init(struct name *table, uint32_t initial_size) {       \
        table->used = 0;                                                \
        table->size = 0;                                                \
        table->entries = 0;                                             \
        name##_resize(table, initial_size);                             \
    }                                                                   \
    void name##_free(struct name *table) {                              \
        free(table->entries);                                           \
        table->used = 0;                                                \
        table->size = 0;                                                \
        table->entries = 0;                                             \
    }

#endif
