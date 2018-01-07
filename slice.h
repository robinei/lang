#ifndef SLICE_H
#define SLICE_H

#include "defs.h"
#include "hashtable.h"

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;

DECL_HASH_TABLE(slice_table, slice_t, void *)

int slice_cmp(slice_t a, slice_t b);
int slice_str_cmp(slice_t a, char *b_str);

/* requires that both slices point into the same buffer */
slice_t slice_span(slice_t a, slice_t b);

#endif
