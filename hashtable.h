
#if !defined(EXPAND_INTERFACE) && !defined(EXPAND_IMPLEMENTATION)
#error Define EXPAND_INTERFACE or EXPAND_IMPLEMENTATION before including this header
#endif

#ifndef NAME
#error Define NAME before including this header
#endif
#ifndef KEY_TYPE
#error Define KEY_TYPE before including this header
#endif
#ifndef VALUE_TYPE
#error Define VALUE_TYPE before including this header
#endif

#define CONCAT_SYMBOLS2(x, y) x ## y
#define CONCAT_SYMBOLS(x, y) CONCAT_SYMBOLS2(x, y)
#define NAMESPACED(x) CONCAT_SYMBOLS(NAME, _ ## x)


#ifdef EXPAND_INTERFACE
#undef EXPAND_INTERFACE

#include <stdint.h>

struct NAMESPACED(entry) {
    uint32_t hash;
    KEY_TYPE key;
    VALUE_TYPE value;
};

struct NAME {
    uint32_t used, size;
    struct NAMESPACED(entry) *entries;
};

void NAMESPACED(clear)(struct NAME *table);
int NAMESPACED(remove)(struct NAME *table, KEY_TYPE key);
int NAMESPACED(find)(struct NAME *table, KEY_TYPE key, uint32_t *index_out);
int NAMESPACED(get)(struct NAME *table, KEY_TYPE key, VALUE_TYPE *value_out);
void NAMESPACED(put)(struct NAME *table, KEY_TYPE key, VALUE_TYPE value);
void NAMESPACED(init)(struct NAME *table, uint32_t initial_size);
void NAMESPACED(free)(struct NAME *table);

#endif /* EXPAND_INTERFACE */



#ifdef EXPAND_IMPLEMENTATION
#undef EXPAND_IMPLEMENTATION

#ifndef HASH_FUNC
#error Define HASH_FUNC before including this header with EXPAND_IMPLEMENTATION defined
#endif
#ifndef EQUAL_FUNC
#error Define EQUAL_FUNC before including this header with EXPAND_IMPLEMENTATION defined
#endif


#ifndef HASH_TABLE_COMMON
#define HASH_TABLE_COMMON

#include <stdlib.h>
#include <assert.h>

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

#endif /* HASH_TABLE_COMMON */


void NAMESPACED(clear)(struct NAME *table) {
    for (uint32_t i = 0; i < table->size; ++i) {
        table->entries[i].hash = 0;
    }
    table->used = 0;
}

static uint32_t NAMESPACED(calc_hash)(KEY_TYPE key) {
    uint32_t hash = HASH_FUNC(key);
    return hash ? hash : 1;
}

int NAMESPACED(find)(struct NAME *table, KEY_TYPE key, uint32_t *index_out) {
    assert(table->used <= table->size);
    if (table->used == 0) {
        return 0;
    }
    uint32_t hash = NAMESPACED(calc_hash)(key);
    uint32_t start_index = hash & (table->size - 1);
    for (uint32_t i = 0; i < table->size; ++i) {
        uint32_t index = (start_index + i) & (table->size - 1);
        struct NAMESPACED(entry) *slot = table->entries + index;
        if (slot->hash == hash && EQUAL_FUNC(slot->key, key)) {
            *index_out = index;
            return 1;
        }
        if (slot->hash == 0) {
            break;
        }
        uint32_t d = hashutil_dist_to_start(table->size, slot->hash, index);
        if (i > d) {
            break;
        }
    }
    return 0;
}

static void NAMESPACED(put_entry)(struct NAME *table, struct NAMESPACED(entry) entry) {
    uint32_t start_index = entry.hash & (table->size - 1);
    uint32_t probe = 0;
    for (uint32_t i = 0; i < table->size; ++i, ++probe) {
        uint32_t index = (start_index + i) & (table->size - 1);
        struct NAMESPACED(entry) *slot = table->entries + index;
        if (slot->hash == 0) {
            ++table->used;
            *slot = entry;
            return;
        }
        if (slot->hash == entry.hash && EQUAL_FUNC(slot->key, entry.key)) {
            slot->value = entry.value;
            return;
        }
        uint32_t slot_probe = hashutil_dist_to_start(table->size, slot->hash, index);
        if (probe > slot_probe) {
            struct NAMESPACED(entry) temp = entry;
            entry = *slot;
            *slot = temp;
            probe = slot_probe;
        }
    }
    assert(0 && "control flow should not get here");
}

int NAMESPACED(remove)(struct NAME *table, KEY_TYPE key) {
    struct NAMESPACED(entry) temp;
    uint32_t index;
    if (!NAMESPACED(find)(table, key, &index)) {
        return 0;
    }
    for (uint32_t i = 0; i < table->size; ++i) {
        uint32_t curr_index = (index + i) & (table->size - 1);
        uint32_t next_index = (index + i + 1) & (table->size - 1);
        uint32_t next_hash = table->entries[next_index].hash;
        if (next_hash == 0 || hashutil_dist_to_start(table->size, next_hash, next_index) == 0) {
            table->entries[curr_index].hash = 0;
            --table->used;
            return 1;
        }
        temp = table->entries[curr_index];
        table->entries[curr_index] = table->entries[next_index];
        table->entries[next_index] = temp;
    }
    assert(0 && "control flow should not get here");
    return 0;
}

static void NAMESPACED(resize)(struct NAME *table, uint32_t new_size) {
    assert(new_size > 0);
    uint32_t old_used = table->used;
    uint32_t old_size = table->size;
    struct NAMESPACED(entry) *old_entries = table->entries;
    table->used = 0;
    table->size = new_size;
    table->entries = calloc(1, sizeof(struct NAMESPACED(entry)) * new_size);
    if (old_used) {
        assert(old_used <= new_size);
        for (uint32_t i = 0; i < old_size; ++i) {
            struct NAMESPACED(entry) *slot = old_entries + i;
            if (slot->hash) {
                NAMESPACED(put_entry)(table, *slot);
            }
        }
        free(old_entries);
    }
}

int NAMESPACED(get)(struct NAME *table, KEY_TYPE key, VALUE_TYPE *value_out) {
    uint32_t index;
    if (NAMESPACED(find)(table, key, &index)) {
        *value_out = table->entries[index].value;
        return 1;
    }
    return 0;
}

void NAMESPACED(put)(struct NAME *table, KEY_TYPE key, VALUE_TYPE value) {
    struct NAMESPACED(entry) entry;
    if (!table->size || (float)table->used / table->size > 0.85f) {
        NAMESPACED(resize)(table, table->size ? table->size * 2 : 16);
    }
    entry.hash = NAMESPACED(calc_hash)(key);
    entry.key = key;
    entry.value = value;
    NAMESPACED(put_entry)(table, entry);
}

void NAMESPACED(init)(struct NAME *table, uint32_t initial_size) {
    table->used = 0;
    table->size = 0;
    table->entries = NULL;
    NAMESPACED(resize)(table, initial_size);
}

void NAMESPACED(free)(struct NAME *table) {
    free(table->entries);
    table->used = 0;
    table->size = 0;
    table->entries = NULL;
}

#endif /* EXPAND_IMPLEMENTATION */


#undef CONCAT_SYMBOLS
#undef CONCAT_SYMBOLS2
#undef NAMESPACED

#undef NAME
#undef KEY_TYPE
#undef VALUE_TYPE
#undef HASH_FUNC
#undef EQUAL_FUNC
