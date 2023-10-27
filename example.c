#include <stdio.h>
#include "src/static_prefixtree.h"

int main() {
    arena a, temp;
    arena_init(&a);
    arena_init(&temp);

    char *strings[] = { "Apple", "App", "cool", "abcdefghijklmnopqrstuvwxyz" };

    printf("Building\n");
    static_prefixtree_node *root = prefixtree_build(&a, temp, strings, sizeof(strings) / sizeof(strings[0]));
    printf("Built\n");

    // interactive traverser
    static_prefixtree_node *node = root;
    while (true) {
        int c = fgetc(stdin);
        if (c == EOF) {
            break;
        } else if (c == '\n') {
            printf("result %d\n", node ? prefixtree_is_target(node) : -1);
            node = root;
            continue;
        }
        node = node ? prefixtree_get_child(node, (char)c) : NULL;
    }
}
