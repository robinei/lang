#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void *arena_alloc(struct arena *arena, uint size) {
    /* round up to nearest 8 so subsequent allocations will remain aligned to that */
    size += ((size & 3) != 0) * (8 - (size & 3));
    struct arena_buffer *buf = arena->buffers;
    uint buf_size = arena->buffer_size;
    if (!buf || buf->used + size > buf_size) {
        if (buf_size == 0) {
            arena->buffer_size = buf_size = 1024*1024;
        }
        if (size > buf_size) {
            buf_size = size;
        }
        buf = calloc(1, sizeof(struct arena_buffer) + buf_size);
        buf->next = arena->buffers;
        arena->buffers = buf;
    }
    char *ptr = buf->data + buf->used;
    buf->used += size;
    return ptr;
}

struct arena_mark arena_mark_allocated(struct arena *arena) {
    return (struct arena_mark) {
        .buffer = arena->buffers,
        .used = arena->buffers ? arena->buffers->used : 0
    };
}

void arena_reset_to_mark(struct arena *arena, struct arena_mark mark) {
    if (!mark.buffer) {
        arena_dealloc_all(arena);
        return;
    }
    while (arena->buffers) {
        if (arena->buffers == mark.buffer) {
            arena->buffers->used = mark.used;
            return;
        }
        struct arena_buffer *next = arena->buffers->next;
        free(arena->buffers);
        arena->buffers = next;
    }
    assert(0 && "marked buffer not present in arena");
}

void arena_dealloc_all(struct arena *arena) {
    while (arena->buffers) {
        struct arena_buffer *next = arena->buffers->next;
        free(arena->buffers);
        arena->buffers = next;
    }
}
