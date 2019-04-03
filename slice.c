#include "sym.h"
#include "murmur3.h"
#include "fnv.h"
#include <stdlib.h>
#include <string.h>

slice_t slice_from_str(char *str) {
    return (slice_t) {
        .ptr = str,
        .len = strlen(str)
    };
}

slice_t slice_from_sym(struct symbol *sym) {
    return (slice_t) {
        .ptr = sym->data,
        .len = sym->length
    };
}

bool slice_equals(slice_t a, slice_t b) {
    if (a.len != b.len) {
        return false;
    }
    return !memcmp(a.ptr, b.ptr, a.len);
}

int slice_cmp(slice_t a, slice_t b) {
    uint min_len = a.len <= b.len ? a.len : b.len;
    uint res = memcmp(a.ptr, b.ptr, min_len);
    return res != 0 ? res : a.len - b.len;
}

int slice_str_cmp(slice_t a, char *b_str) {
    slice_t b = { b_str, strlen(b_str) };
    return slice_cmp(a, b);
}

slice_t slice_span(slice_t a, slice_t b) {
    slice_t c;
    c.ptr = a.ptr <= b.ptr ?
        a.ptr :
        b.ptr;
    c.len = a.ptr + a.len >= b.ptr + b.len ?
        (a.ptr + a.len) - c.ptr :
        (b.ptr + b.len) - c.ptr;
    return c;
}

uint slice_hash_murmur(slice_t s) {
    uint hash;
    MurmurHash3_x86_32(s.ptr, s.len, 0, &hash);
    return hash;
}

uint slice_hash_fnv1a(slice_t s) {
    return fnv1a((unsigned char *)s.ptr, s.len, FNV_SEED);
}


#define EXPAND_IMPLEMENTATION
#define NAME        slice_table
#define KEY_TYPE    slice_t
#define VALUE_TYPE  void *
#define HASH_FUNC   slice_hash_murmur
#define EQUAL_FUNC  slice_equals
#include "hashtable.h"
