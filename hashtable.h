#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "alloc.h"
#include <stdlib.h>
#include <assert.h>

#define HASHTABLE_NAME(N, K, V, HF, EQ) N
#define HASHTABLE_ENTRYNAME(N, K, V, HF, EQ) N##_entry
#define HASHTABLE_KEYTYPE(N, K, V, HF, EQ) K
#define HASHTABLE_VALUETYPE(N, K, V, HF, EQ) V
#define HASHTABLE_HASHFUNC(N, K, V, HF, EQ) HF
#define HASHTABLE_EQFUNC(N, K, V, HF, EQ) EQ

#define DECLARE_HASHTABLE(Params) \
    struct Params(HASHTABLE_ENTRYNAME) { \
        uint32_t hash; \
        Params(HASHTABLE_KEYTYPE) key; \
        Params(HASHTABLE_VALUETYPE) value; \
    }; \
    struct Params(HASHTABLE_NAME) { \
        struct allocator *alloc; \
        uint32_t used, size; \
        struct Params(HASHTABLE_ENTRYNAME) *entries; \
    };

#define hashtable_find(Params, Table, Key, IndexOut, FoundOut) \
    do { \
        assert((Table).used <= (Table).size); \
        FoundOut = false; \
        if ((Table).used > 0) { \
            Params(HASHTABLE_KEYTYPE) _key = Key; \
            uint32_t _hash = Params(HASHTABLE_HASHFUNC)(_key); \
            uint32_t _start_index = _hash & ((Table).size - 1); \
            for (uint32_t _i = 0; _i < (Table).size; ++_i) { \
                uint32_t _curr_index = (_start_index + _i) & ((Table).size - 1); \
                struct Params(HASHTABLE_ENTRYNAME) *_slot = (Table).entries + _curr_index; \
                if (_slot->hash == _hash && Params(HASHTABLE_EQFUNC)(_slot->key, _key)) { IndexOut = _curr_index; FoundOut = true; break; } \
                if (_slot->hash == 0) break; \
                if (_i > _hashtable_dist_to_start((Table).size, _slot->hash, _curr_index)) break; \
            } \
        } \
    } while(0)

#define hashtable_putentry(Params, Table, Entry) \
    do { \
        struct Params(HASHTABLE_ENTRYNAME) _entry = Entry; \
        uint32_t _start_index = _entry.hash & ((Table).size - 1); \
        uint32_t _probe = 0; \
        uint32_t _i = 0; \
        for (; _i < (Table).size; ++_i, ++_probe) { \
            uint32_t _index = (_start_index + _i) & ((Table).size - 1); \
            struct Params(HASHTABLE_ENTRYNAME) *_slot = (Table).entries + _index; \
            if (_slot->hash == 0) { \
                ++(Table).used; \
                *_slot = _entry; \
                break; \
            } \
            if (_slot->hash == _entry.hash && Params(HASHTABLE_EQFUNC)(_slot->key, _entry.key)) { \
                _slot->value = _entry.value; \
                break; \
            } \
            uint32_t _slot_probe = _hashtable_dist_to_start((Table).size, _slot->hash, _index); \
            if (_probe > _slot_probe) { \
                struct Params(HASHTABLE_ENTRYNAME) _temp = _entry; \
                _entry = *_slot; \
                *_slot = _temp; \
                _probe = _slot_probe; \
            } \
        } \
        assert(_i < (Table).size && "unexpectedly could not find a slot for insertion"); \
    } while (0)

#define hashtable_remove(Params, Table, Key, RemovedOut) \
    do { \
        RemovedOut = false; \
        uint32_t _index; \
        bool _found; \
        hashtable_find(Params, Table, Key, _index, _found); \
        if (_found) { \
            uint32_t _i = 0; \
            for (; _i < (Table).size; ++_i) { \
                uint32_t _curr_index = (_index + _i) & ((Table).size - 1); \
                uint32_t _next_index = (_index + _i + 1) & ((Table).size - 1); \
                uint32_t _next_hash = (Table).entries[_next_index].hash; \
                if (_next_hash == 0 || _hashtable_dist_to_start((Table).size, _next_hash, _next_index) == 0) { \
                    (Table).entries[_curr_index].hash = 0; \
                    --(Table).used; \
                    RemovedOut = true; \
                    break; \
                } \
                struct Params(HASHTABLE_ENTRYNAME) _temp = (Table).entries[_curr_index]; \
                (Table).entries[_curr_index] = (Table).entries[_next_index]; \
                (Table).entries[_next_index] = _temp; \
            } \
            assert(_i < (Table).size && "unexpectedly could finish removal"); \
        } \
    } while(0)

#define hashtable_resize(Params, Table, NewSize) \
    do { \
        uint32_t _new_size = (NewSize); \
        assert(_new_size > 0); \
        uint32_t _old_used = (Table).used; \
        uint32_t _old_size = (Table).size; \
        struct Params(HASHTABLE_ENTRYNAME) *_old_entries = (Table).entries; \
        (Table).used = 0; \
        (Table).size = _new_size; \
        (Table).entries = allocate((Table).alloc, sizeof(struct Params(HASHTABLE_ENTRYNAME)) * _new_size); \
        if (_old_used) { \
            assert(_old_used <= _new_size); \
            for (uint32_t _i = 0; _i < _old_size; ++_i) { \
                struct Params(HASHTABLE_ENTRYNAME) *_slot = _old_entries + _i; \
                if (_slot->hash) { \
                    hashtable_putentry(Params, Table, *_slot); \
                } \
            } \
            deallocate((Table).alloc, _old_entries, sizeof(struct Params(HASHTABLE_ENTRYNAME)) * _old_size); \
        } \
    } while(0)

#define hashtable_get(Params, Table, Key, ValueOut, FoundOut) \
    do { \
        uint32_t _index; \
        hashtable_find(Params, Table, Key, _index, FoundOut); \
        if (FoundOut) { \
            ValueOut = (Table).entries[_index].value; \
        } \
    } while(0)

#define hashtable_put(Params, Table, Key, Value) \
    do { \
        struct Params(HASHTABLE_ENTRYNAME) _new_entry; \
        if (!(Table).size || (float)(Table).used / (Table).size > 0.85f) { \
            hashtable_resize(Params, Table, (Table).size ? (Table).size * 2 : 16); \
        } \
        _new_entry.key = Key; \
        _new_entry.value = Value; \
        _new_entry.hash = Params(HASHTABLE_HASHFUNC)(_new_entry.key); \
        hashtable_putentry(Params, Table, _new_entry); \
    } while(0)

#define hashtable_init(Params, Table, Allocator, InitialSize) \
    do { \
        (Table).alloc = Allocator; \
        (Table).used = 0; \
        (Table).size = 0; \
        (Table).entries = NULL; \
        if ((InitialSize) > 0) { \
            hashtable_resize(Params, Table, InitialSize); \
        } \
    } while(0)

#define hashtable_cleanup(Params, Table) \
    do { \
        deallocate((Table).alloc, (Table).entries, sizeof(struct Params(HASHTABLE_ENTRYNAME)) * (Table).size); \
        (Table).used = 0; \
        (Table).size = 0; \
        (Table).entries = NULL; \
    } while(0)

#define hashtable_clear(Params, Table) \
    do { \
        for (uint32_t i = 0; i < (Table).size; ++i) (Table).entries[i].hash = 0; \
        (Table).used = 0; \
    } while(0)

static inline uint32_t _hashtable_dist_to_start(uint32_t table_size, uint32_t hash, uint32_t index_stored) {
    assert(hash);
    uint32_t start_index = hash & (table_size - 1);
    if (start_index <= index_stored) {
        return index_stored - start_index;
    }
    return index_stored + (table_size - start_index);
}

static uint32_t calc_uint32_hash(uint32_t key) {
    key = (key ^ 61) ^ (key >> 16);
    key = key + (key << 3);
    key = key ^ (key >> 4);
    key = key * 0x27d4eb2d;
    key = key ^ (key >> 15);
    return key;
}

static uint32_t calc_ptr_hash(void *ptr) {
    uint32_t val = (uint32_t)(intptr_t)ptr;
    val = ~val + (val << 15);
    val = val ^ (val >> 12);
    val = val + (val << 2);
    val = val ^ (val >> 4);
    val = val * 2057;
    val = val ^ (val >> 16);
    return val;
}

#define VALUE_EQ(X, Y) ((X) == (Y))

#endif
