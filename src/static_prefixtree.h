#ifndef _WING_PREFIXTREE
#define _WING_PREFIXTREE

#include "arena.h"

// an ASCII prefix-tree allocated once in an arena of its own, to be static from that point onwards
typedef struct static_prefixtree_node {

    // 127 potential childrens in ASCII - null byte is illegal
    // child_bits[0] is set for non-internal (target) nodes
    uint64_t child_bits[2];

    // dynamically sized array, length defined by number of bits in child_bits
    struct static_prefixtree_node *children[];

} static_prefixtree_node;

// Build a static prefixtree in a given allocator and with a parent allocator to fork for temporary allocations during building
// returns NULL on error or empty
static_prefixtree_node *prefixtree_build(arena *a, arena temp_parent, char *strings[], size_t strings_count);


// Accessor methods
static_prefixtree_node *prefixtree_get_child(static_prefixtree_node *node, char c);
static inline bool prefixtree_is_target(static_prefixtree_node *node);


// Inline implementations
static inline bool prefixtree_is_target(static_prefixtree_node *node) {
    return node->child_bits[0] & 1;
}

#endif
