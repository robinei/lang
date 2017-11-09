#ifndef FNV_H
#define FNV_H

#include "defs.h"

#define FNV_PRIME 0x01000193
#define FNV_SEED 0x811C9DC5

static uint fnv1a(unsigned char *ptr, uint len, uint hash) {
    while (len--) {
        hash = (*ptr++ ^ hash) * FNV_PRIME;
    }
    return hash;
}

#endif
