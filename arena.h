#ifndef ARENA_H
#define ARENA_H

#include "defs.h"

struct arena_buffer {
    struct arena_buffer *next;
    uint used;
    char data[0];
};

struct arena_mark {
    struct arena_buffer *buffer;
    uint used;
};

struct arena {
    uint buffer_size;
    struct arena_buffer *buffers;
};

void *arena_alloc(struct arena *arena, uint size);
struct arena_mark arena_mark_allocated(struct arena *arena);
void arena_reset_to_mark(struct arena *arena, struct arena_mark mark);
void arena_dealloc_all(struct arena *arena);

#endif
