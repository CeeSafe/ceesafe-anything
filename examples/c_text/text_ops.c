#include <stddef.h>
#include <stdint.h>

int32_t word_count(const char *s) {
    if (s == NULL) {
        return 0;
    }
    int32_t count = 0;
    int in_word = 0;
    for (size_t i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        int is_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
        if (!is_space && !in_word) {
            count++;
            in_word = 1;
        } else if (is_space) {
            in_word = 0;
        }
    }
    return count;
}

int32_t to_upper_ascii(uint8_t *out, size_t len) {
    if (out == NULL) {
        return 0;
    }
    int32_t changed = 0;
    for (size_t i = 0; i < len; i++) {
        if (out[i] >= 'a' && out[i] <= 'z') {
            out[i] = (uint8_t)(out[i] - 32);
            changed++;
        }
    }
    return changed;
}

int32_t count_char(const uint8_t *ptr, size_t len, int32_t target) {
    if (ptr == NULL) {
        return 0;
    }
    int32_t count = 0;
    for (size_t i = 0; i < len; i++) {
        if ((int32_t)ptr[i] == target) {
            count++;
        }
    }
    return count;
}
