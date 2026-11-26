#include <stdint.h>

typedef enum {
    SHAPE_RECT = 0,
    SHAPE_TRIANGLE = 1,
} ShapeKind;

typedef struct {
    int32_t width;
    int32_t height;
} Rect2D;

int32_t rect_area(const Rect2D *r) {
    if (r == 0) {
        return 0;
    }
    return r->width * r->height;
}

int32_t shape_area(ShapeKind kind, int32_t w, int32_t h) {
    if (kind == SHAPE_TRIANGLE) {
        return (w * h) / 2;
    }
    return w * h;
}
