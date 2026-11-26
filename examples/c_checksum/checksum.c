#include <stddef.h>
#include <stdint.h>

/* Sample C unit for end-to-end ceesafe-anything smoke testing. */
int32_t port_checksum_run(const uint8_t *ptr, size_t len) {
    uint32_t acc = 0;
    if (!ptr) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        acc = (acc * 31u + ptr[i]) & 0x7fffffffu;
    }
    return (int32_t)acc;
}
