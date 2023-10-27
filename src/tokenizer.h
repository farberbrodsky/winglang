#ifndef _WING_TOKENIZER
#define _WING_TOKENIZER

#include "util.h"
#include "static_prefixtree.h"

typedef struct {
    static_prefixtree_node *tokens_tree_root;
    const char *text_position;
    static_prefixtree_node *state;
    int line;  // 1-based
} tokenizer_ctx;

void tokenizer_init(tokenizer_ctx *dst, const char *source, static_prefixtree_node *tokens_tree);
// return NULL on EOF, line number updates to end of token
void *tokenizer_get_token(tokenizer_ctx *ctx);

#endif
