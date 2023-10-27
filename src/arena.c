#include "arena.h"

// inspired by https://nullprogram.com/blog/2023/09/27/ but dynamic size

// arena sizes can be multiplied by 8 without overflow
static size_t arena_max = (size_t)1 << (8 * sizeof(size_t) - 3);

void *arena_alloc_one(arena *a, size_t size, size_t align) {
    size_t available = a->head - a->tail;
    size_t padding = (-(uintptr_t)a->tail) & (align - 1);
    size_t padded_size;

    if (unlikely(__builtin_uaddl_overflow(size, padding, &padded_size))) {
        return NULL;
    }

    if (padded_size > available) {
        // start a new allocation, double the size
        // LAYOUT: previous allocation (void *), data
        size_t prev_allocation_size = a->head - a->allocation_start;

        // check overflow
        assert((size + align) < arena_max);
        static_assert(sizeof(void *) >= 2, "theoretically needed of next_pow2_ul");

        size_t next_allocation_size = next_pow2_ul(sizeof(void *) + prev_allocation_size + size + align);

        // check overflow again
        assert(next_allocation_size < arena_max);

        // do the allocation
        char *next_alloc = malloc(next_allocation_size);
        if (!next_alloc) {
            return NULL;
        }

        // keep layout
        *(void **)next_alloc = a->allocation_start;
        a->allocation_start = next_alloc;
        a->tail = next_alloc + sizeof(void *);
        a->head = next_alloc + next_allocation_size;
        padding = (-(uintptr_t)a->tail) & (align - 1);
        padded_size = size + padding;
    }

    char *p = a->tail + padding;
    a->tail += padded_size;
    return p;
}

void arena_init_reserve(arena *a, size_t size) {
    assert(size < (arena_max - sizeof(void *)));
    size_t full_size = size + sizeof(void *);

    a->allocation_start = malloc(full_size);
    a->tail = a->allocation_start + sizeof(void *);
    a->head = a->allocation_start + full_size;
    *(void **)a->allocation_start = NULL;
}

// arena_free of a non-fork: parent_allocation_start = NULL
void _arena_fork_free_internal(arena *a, char *parent_allocation_start) {
    void *alloc = a->allocation_start;

    while (alloc != parent_allocation_start) {
        void *prev_alloc = *(void **)alloc;
        free(alloc);
        alloc = prev_alloc;
    }
}
