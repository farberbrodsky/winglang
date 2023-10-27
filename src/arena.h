#ifndef _WING_ARENA
#define _WING_ARENA

#include <stdalign.h>
#include "util.h"

// the sizeof(void *) bytes before tail are a pointer to the previous allocation or 0
typedef struct {
    char *allocation_start;
    char *tail;
    char *head;
} arena;


// INITIALIZATION

// initialize the arena without any memory
static inline void arena_init(arena *a);
// initialize the arena with a given size
void arena_init_reserve(arena *a, size_t size);


// DEALLOCATION

// deallocate all objects in arena
static inline void arena_free(arena *a);
// Arenas can be forked - by simply copying an arena object, and remembering to free with the parent
static inline void arena_fork_free(arena *a, arena *parent);


// ALLOCATION FUNCTIONS

// align must be a power of two
// size can't be 0
// returns NULL on error
void *arena_alloc_one               (arena *a, size_t size, size_t align) __attribute__((malloc));
#define ARENA_ALLOC_ONE(a, type)  arena_alloc_one (a, sizeof(type), alignof(type))
static inline void *arena_calloc_one(arena *a, size_t size, size_t align) __attribute__((malloc));
#define ARENA_CALLOC_ONE(a, type) arena_calloc_one(a, sizeof(type), alignof(type))
// align must be a power of two
// size can't be 0
// count can't be 0
// returns NULL on error
static inline void *arena_alloc     (arena *a, size_t size, size_t count, size_t align) __attribute__((malloc));
static inline void *arena_calloc    (arena *a, size_t size, size_t count, size_t align) __attribute__((malloc));
#define ARENA_ALLOC(a, type, count)  arena_alloc (a, sizeof(type), count, alignof(type))
#define ARENA_CALLOC(a, type, count) arena_calloc(a, sizeof(type), count, alignof(type))


// INLINE IMPLEMENTATIONS
static inline void arena_init(arena *a) {
    memset(a, 0, sizeof(arena));
}

// Generalization of fork_free and free
void _arena_fork_free_internal(arena *a, char *parent_allocation_start);

static inline void arena_free(arena *a) {
    _arena_fork_free_internal(a, NULL);
}

static inline void arena_fork_free(arena *a, arena *parent) {
    _arena_fork_free_internal(a, parent->allocation_start);
}

static inline void *arena_calloc_one(arena *a, size_t size, size_t align) {
    void *result = arena_alloc_one(a, size, align);
    if (likely(result)) {
        memset(result, 0, size);
    }
    return result;
}

static inline void *arena_alloc(arena *a, size_t size, size_t count, size_t align) {
    size_t result_size;

    if (unlikely(__builtin_umull_overflow(size, count, &result_size))) {
        return NULL;
    }

    return arena_alloc_one(a, result_size, align);
}

static inline void *arena_calloc(arena *a, size_t size, size_t count, size_t align) {
    void *result = arena_alloc(a, size, count, align);
    if (likely(result)) {
        memset(result, 0, size);
    }
    return result;
}

#endif
