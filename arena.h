#ifndef ARENA_H
#define ARENA_H

#include "defs.h"

struct arena_buffer {
    struct arena_buffer *next;
    uint used;
    char data[0];
};

struct arena {
    uint buffer_size;
    struct arena_buffer *buffers;
};

void *arena_alloc(struct arena *arena, uint size);
void arena_dealloc_all(struct arena *arena);

#endif
