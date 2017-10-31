#include "slice.h"
#include "murmur3.h"
#include "hashtable_impl.h"
#include <stdlib.h>

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
