#include "slice.h"
#include "murmur3.h"
#include "hashtable_impl.h"
#include <stdlib.h>


int slice_cmp(slice_t a, slice_t b) {
    uint min_len = a.len <= b.len ? a.len : b.len;
    uint res = memcmp(a.ptr, b.ptr, min_len);
    return res != 0 ? res : a.len - b.len;
}

int slice_str_cmp(slice_t a, char *b_str) {
    slice_t b = { b_str, strlen(b_str) };
    return slice_cmp(a, b);
}

static uint slice_hash(slice_t s) {
    uint hash;
    MurmurHash3_x86_32(s.ptr, s.len, 0, &hash);
    return hash;
}

static int slice_equals(slice_t a, slice_t b) {
    if (a.len != b.len) {
        return 0;
    }
    return !memcmp(a.ptr, b.ptr, a.len);
}

IMPL_HASH_TABLE(slice_table, slice_t, void *, slice_hash, slice_equals)
