#include "alloc.h"
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>



static void *default_alloc(struct allocator *a, uint size) {
    return calloc(1, size);
}

static void default_dealloc(struct allocator *a, void *ptr, uint size) {
    free(ptr);
}

struct allocator _default_allocator = {
    .alloc_func = default_alloc,
    .dealloc_func = default_dealloc
};

struct allocator *default_allocator = &_default_allocator;



static void *tracking_alloc(struct allocator *a, uint size) {
    struct tracking_allocator *ta = (struct tracking_allocator *)a;
    struct tracking_allocator_entry *e = allocate(ta->base_allocator, sizeof(struct tracking_allocator_entry) + size);
    e->prev = ta->entries.prev;
    e->next = &ta->entries;
    e->prev->next = e;
    e->next->prev = e;
    e->size = size;
    return e->data;
}

static void tracking_dealloc(struct allocator *a, void *ptr, uint size) {
    if (!ptr) {
        return;
    }
    struct tracking_allocator *ta = (struct tracking_allocator *)a;
    struct tracking_allocator_entry *e = (struct tracking_allocator_entry *)((char *)ptr - offsetof(struct tracking_allocator_entry, data));
    assert(e->size == size);
    e->prev->next = e->next;
    e->next->prev = e->prev;
    deallocate(ta->base_allocator, e, sizeof(struct tracking_allocator_entry) + e->size);
}

struct tracking_allocator *tracking_allocator_create(struct allocator *base_allocator) {
    struct tracking_allocator *ta = allocate(base_allocator, sizeof(struct tracking_allocator));
    tracking_allocator_init(ta, base_allocator);
    return ta;
}

void tracking_allocator_destroy(struct tracking_allocator *ta) {
    tracking_allocator_cleanup(ta);
    deallocate(ta->base_allocator, ta, sizeof(struct tracking_allocator));
}

void tracking_allocator_init(struct tracking_allocator *ta, struct allocator *base_allocator) {
    ta->a.alloc_func = tracking_alloc;
    ta->a.dealloc_func = tracking_dealloc;
    ta->base_allocator = base_allocator;
    ta->entries.prev = &ta->entries;
    ta->entries.next = &ta->entries;
}

void tracking_allocator_cleanup(struct tracking_allocator *ta) {
    struct tracking_allocator_entry *e = ta->entries.prev;
    struct tracking_allocator_entry *head = &ta->entries;

    while (e != head) {
        struct tracking_allocator_entry *prev = e->prev;
        deallocate(ta->base_allocator, e, sizeof(struct tracking_allocator_entry) + e->size);
        e = prev;
    }

    ta->entries.prev = head;
    ta->entries.next = head;
}



static void *arena_alloc(struct allocator *a, uint size) {
    struct arena_allocator *arena = (struct arena_allocator *)a;
    struct arena_buffer *buf = arena->buffers;
    uint buf_size = arena->buffer_size;
    
    /* round up to nearest 8 so subsequent allocations will remain aligned to that */
    size += ((size & 3) != 0) * (8 - (size & 3));
    
    if (!buf || buf->used + size > buf_size) {
        if (buf_size == 0) {
            arena->buffer_size = buf_size = 1024*1024;
        }
        if (size > buf_size) {
            buf_size = size;
        }
        buf = allocate(arena->base_allocator, sizeof(struct arena_buffer) + buf_size);
        buf->next = arena->buffers;
        arena->buffers = buf;
    }

    char *ptr = buf->data + buf->used;
    buf->used += size;
    return ptr;
}

static void arena_dealloc(struct allocator *a, void *ptr, uint size) {
    /* can't dealloc individual arena allocation */
}

struct arena_allocator *arena_allocator_create(struct allocator *base_allocator, uint buffer_size) {
    struct arena_allocator *arena = allocate(base_allocator, sizeof(struct arena_allocator));
    arena_allocator_init(arena, base_allocator, buffer_size);
    return arena;
}

void arena_allocator_destroy(struct arena_allocator *arena) {
    arena_allocator_cleanup(arena);
    deallocate(arena->base_allocator, arena, sizeof(struct arena_allocator));
}

void arena_allocator_init(struct arena_allocator *arena, struct allocator *base_allocator, uint buffer_size) {
    arena->a.alloc_func = arena_alloc;
    arena->a.dealloc_func = arena_dealloc;
    arena->base_allocator = base_allocator;
    arena->buffers = NULL;
    arena->buffer_size = buffer_size;
}

void arena_allocator_cleanup(struct arena_allocator *arena) {
    uint buf_size = arena->buffer_size;
    while (arena->buffers) {
        struct arena_buffer *next = arena->buffers->next;
        deallocate(arena->base_allocator, arena->buffers, sizeof(struct arena_buffer) + buf_size);
        arena->buffers = next;
    }
}

struct arena_mark arena_allocator_mark(struct arena_allocator *arena) {
    return (struct arena_mark) {
        .buffer = arena->buffers,
        .used = arena->buffers ? arena->buffers->used : 0
    };
}

void arena_allocator_reset(struct arena_allocator *arena, struct arena_mark mark) {
    if (!mark.buffer) {
        arena_allocator_cleanup(arena);
        return;
    }
    uint buf_size = arena->buffer_size;
    while (arena->buffers) {
        if (arena->buffers == mark.buffer) {
            arena->buffers->used = mark.used;
            return;
        }
        struct arena_buffer *next = arena->buffers->next;
        deallocate(arena->base_allocator, arena->buffers, sizeof(struct arena_buffer) + buf_size);
        arena->buffers = next;
    }
    assert(0 && "marked buffer not present in arena");
}
