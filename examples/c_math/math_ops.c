#include <stddef.h>
#include <stdint.h>

/* Multi-function C unit for testing automatic signature extraction and differential diffing. */
int32_t add_i32(int32_t a, int32_t b) {
    return a + b;
}

int32_t clamp_i32(int32_t x, int32_t lo, int32_t hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

int32_t hash_bytes(const uint8_t *ptr, size_t len) {
    uint32_t acc = 0;
    if (!ptr) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        acc = (acc * 31u + ptr[i]) & 0x7fffffffu;
    }
    return (int32_t)acc;
}
