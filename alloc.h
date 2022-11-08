#ifndef ALLOC_H
#define ALLOC_H

#include "defs.h"
#include <string.h>



/*
 * An allocator just provides an interface for alloc and a dealloc functions.
 * Concrete allocators must keep the basic allocator struct as their first member.
 */

struct allocator;
typedef void *(*alloc_func_t)(struct allocator *a, uint size);
typedef void (*dealloc_func_t)(struct allocator *a, void *ptr, uint size);

struct allocator {
    alloc_func_t alloc_func;
    dealloc_func_t dealloc_func;
};

static inline void *allocate(struct allocator *a, uint size) {
    void *ptr = a->alloc_func(a, size);
    memset(ptr, 0, size); // extra precaution
    return ptr;
}
static inline void deallocate(struct allocator *a, void *ptr, uint size) {
    a->dealloc_func(a, ptr, size);
}

/* this is a wrapper around the stdlib allocator */
extern struct allocator *default_allocator;



/*
 * The tracking allocator wraps another allocator, and tracks all allocations.
 * It provides the capability of deallocating all tracked allocations, that have not
 * been explicitly deallocated.
 */ 

struct tracking_allocator_entry {
    struct tracking_allocator_entry *prev, *next;
    uint size;
    char data[0];
};

struct tracking_allocator {
    struct allocator a;
    struct allocator *base_allocator;
    struct tracking_allocator_entry entries;
};

struct tracking_allocator *tracking_allocator_create(struct allocator *base_allocator);
void tracking_allocator_destroy(struct tracking_allocator *ta);

void tracking_allocator_init(struct tracking_allocator *ta, struct allocator *base_allocator);
void tracking_allocator_cleanup(struct tracking_allocator *ta);



/*
 * The arena allocator uses a base allocator to provide large buffers from which it
 * allocates very cheaply using "pointer bumping".
 * It cannot deallocate individual allocations. Instead the whole arena must be deallocated
 * at once (or all allocations back to a stored mark).
 */

#define DEFAULT_ARENA_BUFFER_SIZE 1048576

struct arena_buffer {
    struct arena_buffer *next;
    uint used;
    char data[0];
};

struct arena_mark {
    struct arena_buffer *buffer;
    uint used;
};

struct arena_allocator {
    struct allocator a;
    struct allocator *base_allocator;
    struct arena_buffer *buffers;
    uint buffer_size;
};

struct arena_allocator *arena_allocator_create(struct allocator *base_allocator, uint buffer_size);
void arena_allocator_destroy(struct arena_allocator *arena);

void arena_allocator_init(struct arena_allocator *arena, struct allocator *base_allocator, uint buffer_size);
void arena_allocator_cleanup(struct arena_allocator *arena);

struct arena_mark arena_allocator_mark(struct arena_allocator *arena);
void arena_allocator_reset(struct arena_allocator *arena, struct arena_mark mark);



#endif
