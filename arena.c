#include "arena.h"
#include <stdlib.h>
#include <string.h>

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

void arena_dealloc_all(struct arena *arena) {
    struct arena_buffer *buf = arena->buffers;
    while (buf) {
        struct arena_buffer *next = buf->next;
        free(buf);
        buf = next;
    }
    memset(arena, 0, sizeof(struct arena));
}
