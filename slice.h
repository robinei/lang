#ifndef SLICE_H
#define SLICE_H

#include "alloc.h"

struct symbol;

typedef struct slice {
    char *ptr;
    uint len;
    uint cap;
} slice_t;

slice_t slice_from_str_len(char *str, uint len);
slice_t slice_from_str(char *str);
slice_t slice_from_sym(struct symbol *sym);
bool slice_equals(slice_t a, slice_t b);
int slice_cmp(slice_t a, slice_t b);
int slice_str_cmp(slice_t a, char *b_str);
slice_t slice_dup(slice_t s, struct allocator *a); /* result is 0-terminated */

/* requires that both slices point into the same buffer */
slice_t slice_span(slice_t a, slice_t b);

uint slice_hash_murmur(slice_t s);
uint slice_hash_fnv1a(slice_t s);

#endif
