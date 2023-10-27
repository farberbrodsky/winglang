#include <stdio.h>
#include "../src/static_prefixtree.h"

int main() {
    arena a, temp;
    arena_init(&a);
    arena_init(&temp);

    char *strings[] = { "Apple", "App", "cool", "abcdefghijklmnopqrstuvwxyz" };
    char *user_data[] = { "A fruit", "Short for application", "Moderately cold", "The alphabet" };

    printf("Building\n");
    static_prefixtree_node *root = prefixtree_build(&a, temp, strings, (void **)user_data, sizeof(strings) / sizeof(strings[0]));
    printf("Built\n");

    // interactive traverser
    static_prefixtree_node *node = root;
    while (true) {
        int c = fgetc(stdin);
        if (c == EOF) {
            break;
        } else if (c == '\n') {
            if (node && prefixtree_is_target(node)) {
                printf("%s\n", (char *)prefixtree_get_user_data(node));
            } else if (node) {
                printf("Intermediate node\n");
            } else {
                printf("Not a prefix\n");
            }
            node = root;
            continue;
        }
        node = node ? prefixtree_get_child(node, (char)c) : NULL;
    }
}
