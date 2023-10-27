#include "static_prefixtree.h"

typedef struct wasteful_prefixtree_node {
    uint64_t child_bits[2];
    struct wasteful_prefixtree_node *children[128];
} wasteful_prefixtree_node;

static_assert((1 << 6) == 64, "64 has 6 bits");

// utility functions for both node types

// INCLUDING the void* for target nodes
static uint get_num_children(uint64_t child_bits[2]) {
    return popcount_ul(child_bits[0]) + popcount_ul(child_bits[1]); // - (child_bits[0] & 1) to exclude the void*
}

// NOT INCLUDING the void* for target nodes
static bool have_no_children(uint64_t child_bits[2]) {
    return child_bits[0] <= 1 && child_bits[1] == 0;
}

// Returns 0 if there's no children
// Otherwise returns a child index, while removing it from the bitset
// 0 is assumed not to be a member
static char pop_some_child(uint64_t child_bits[2]) {
    // check both child_bits

    for (int i = 0; i < 2; i++) {
        if (child_bits[i] != 0) {
            int bit_idx = __builtin_ctzl(child_bits[i]);
            child_bits[i] ^= (uint64_t)1 << bit_idx;
            return (char)(64 * i + bit_idx);
        }
    }

    return 0;
}

static uint get_child_index(uint64_t child_bits[2], char c) {
    if (c < 64) {
        uint64_t prev_bits_0 = child_bits[0] & (((uint64_t)1 << c) - 1);
        return popcount_ul(prev_bits_0);
    } else {
        uint64_t prev_bits_1 = child_bits[1] & (((uint64_t)1 << (c - 64)) - 1);
        return popcount_ul(child_bits[0]) + popcount_ul(prev_bits_1);
    }
}

// prefixtree implementation
static_prefixtree_node *prefixtree_build(arena *a, arena temp_parent, char *strings[], void *user_data[], size_t strings_count) {
    static_prefixtree_node *result = NULL;

    // fork parent temporaries allocator
    arena temp = temp_parent;
    // fork parent allocator in case of error
    arena fork_a = *a;

    // start by building a wasteful prefixtree
    wasteful_prefixtree_node root;
    memset(&root, 0, sizeof(root));
    // store depth for use later
    size_t depth = 0;

    for (size_t strings_i = 0; strings_i < strings_count; strings_i++) {
        char *str = strings[strings_i];

        char c;
        wasteful_prefixtree_node *node = &root;
        size_t str_i = 0;
        while ((c = str[str_i])) {
            // make sure it's all ASCII
            assert((unsigned char)c < 128);

            if (node->children[c] == NULL) {
                // allocate new child
                wasteful_prefixtree_node *new_node = ARENA_CALLOC_ONE(&temp, wasteful_prefixtree_node);
                if (unlikely(!new_node)) goto abort_during_wasteful;

                node->children[c] = new_node;
                node->child_bits[c >> 6] |= (uint64_t)1 << (c & 63);
            }

            // traverse to child
            node = node->children[c];

            // advance in str
            ++str_i;
        }

        // final node on path is not internal
        node->child_bits[0] |= 1;
        node->children[0] = user_data[strings_i];

        // update depth
        if (str_i > depth) depth = str_i;
    }

    // empty prefixtree - NULL
    // needed because allocating 0 is not allowed
    if (depth == 0)
        goto successful_result;

    // build compact prefixtree nodes
    // to iterate over wasteful prefixtree, use a string of length depth
    char *iteration_path_chars = ARENA_ALLOC(&temp, char, depth);
    if (unlikely(!iteration_path_chars)) goto abort_during_compact;

    // and store the path's nodes
    struct iteration_path_pair {
        wasteful_prefixtree_node *wasteful_node;
        static_prefixtree_node *static_node;
    };
    struct iteration_path_pair *iteration_path_pairs = ARENA_ALLOC(&temp, struct iteration_path_pair, depth);
    if (unlikely(!iteration_path_pairs)) goto abort_during_compact;

    size_t iter_depth = 0;
    iteration_path_pairs[iter_depth].wasteful_node = &root;

    // loop invariant: wasteful node exists, static node needs to be created

    while (true) {
        wasteful_prefixtree_node *wasteful_node = iteration_path_pairs[iter_depth].wasteful_node;
        size_t num_children = get_num_children(wasteful_node->child_bits);

        // allocate a node with the correct amount of children
        static_prefixtree_node *node = arena_alloc_one(&fork_a, sizeof(static_prefixtree_node) + num_children * sizeof(static_prefixtree_node *), alignof(static_prefixtree_node));
        if (unlikely(!node)) goto abort_during_compact;
        node->child_bits[0] = wasteful_node->child_bits[0];
        node->child_bits[1] = wasteful_node->child_bits[1];
        iteration_path_pairs[iter_depth].static_node = node;

        if (wasteful_node->child_bits[0] & 1) {
            // Copy user data and remove the bit
            node->children[0] = (void *)wasteful_node->children[0];
            wasteful_node->child_bits[0] ^= 1;
        }

        // continue to next node - either child or some parent's next child
        char child_idx = pop_some_child(wasteful_node->child_bits);
        if (!child_idx) {
            // don't have unvisited children - go up, and on the way up, register ourselves in the parent
            while (have_no_children(wasteful_node->child_bits) && iter_depth > 0) {
                // I am the child_idx
                child_idx = iteration_path_chars[iter_depth];

                // Now, go up to be parent
                iter_depth--;
                static_prefixtree_node *parent_node = iteration_path_pairs[iter_depth].static_node;
                wasteful_node = iteration_path_pairs[iter_depth].wasteful_node;

                // Register the child that went up
                parent_node->children[get_child_index(parent_node->child_bits, child_idx)] = node;

                // Update node to be for this depth
                node = parent_node;
            }

            // Once there is a child, go to it
            child_idx = pop_some_child(wasteful_node->child_bits);

            if (child_idx == 0) {
                // wasteful_node has no children, and the loop stopped, so iter_depth == 0 - therefore the loop finished
                break;
            }
        }

        // go down to child
        iter_depth++;
        iteration_path_pairs[iter_depth].wasteful_node = wasteful_node->children[child_idx];
        iteration_path_chars[iter_depth] = child_idx;
    }

    // take the root node
    result = iteration_path_pairs[0].static_node;

    // update a to include the fork
    *a = fork_a;
    goto successful_result;

abort_during_compact:
    // free compact, return NULL
    result = NULL;
    arena_fork_free(&fork_a, a);
successful_result:
abort_during_wasteful:
    // free temporaries
    arena_fork_free(&temp, &temp_parent);
    return result;
}

static_prefixtree_node *prefixtree_get_child(static_prefixtree_node *node, char c) {
    if (unlikely(c <= 0)) {
        return NULL;
    }

    // check that it is a child
    if (c < 64) {
        if (!(node->child_bits[0] & ((uint64_t)1 << c))) return NULL;
    } else {
        if (!(node->child_bits[1] & ((uint64_t)1 << (c - 64)))) return NULL;
    }

    // in that case, return the child
    int idx = get_child_index(node->child_bits, c);
    return node->children[idx];
}
