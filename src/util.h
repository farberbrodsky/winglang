#ifndef _WING_UTIL
#define _WING_UTIL
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)
typedef unsigned int uint;

// for x >= 2 only
static inline int next_pow2_ul(unsigned long x) {
    assert(x >= 2);
    return (uint64_t)1 << ((sizeof(x) * 8) - __builtin_clzl(x - 1));
}

static inline int popcount_ul(unsigned long x) {
    return __builtin_popcountl(x);
}

#endif
