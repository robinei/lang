#ifndef SLICE_H
#define SLICE_H

#include "defs.h"
#include "hashtable.h"

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;

DECL_HASH_TABLE(slice_table, slice_t, void *)

slice_t slice_from_str(char *str);
int slice_equals(slice_t a, slice_t b);
int slice_cmp(slice_t a, slice_t b);
int slice_str_cmp(slice_t a, char *b_str);

/* requires that both slices point into the same buffer */
slice_t slice_span(slice_t a, slice_t b);

uint slice_hash_murmur(slice_t s);
uint slice_hash_fnv1a(slice_t s);

#endif
