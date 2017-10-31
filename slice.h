#ifndef SLICE_H
#define SLICE_H

#include "defs.h"
#include "hashtable.h"

typedef struct slice {
    char *ptr;
    uint len;
} slice_t;

DECL_HASH_TABLE(slice_table, slice_t, void *)

#endif
