
#include "bitmap-blit.h"

#include <cstring>

namespace msdf_atlas {

template <typename T, int N>
void blitSameType(const msdfgen::BitmapRef<T, N> &dst, const msdfgen::BitmapConstRef<T, N> &src, int dx, int dy, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y)
        memcpy(dst(dx, dy+y), src(sx, sy+y), sizeof(T)*N*w);
}

#define BLIT_SAME_TYPE_IMPL(T, N) void blit(const msdfgen::BitmapRef<T, N> &dst, const msdfgen::BitmapConstRef<T, N> &src, int dx, int dy, int sx, int sy, int w, int h) { blitSameType(dst, src, dx, dy, sx, sy, w, h); }

BLIT_SAME_TYPE_IMPL(byte, 1)
BLIT_SAME_TYPE_IMPL(byte, 3)
BLIT_SAME_TYPE_IMPL(byte, 4)
BLIT_SAME_TYPE_IMPL(float, 1)
BLIT_SAME_TYPE_IMPL(float, 3)
BLIT_SAME_TYPE_IMPL(float, 4)

void blit(const msdfgen::BitmapRef<byte, 1> &dst, const msdfgen::BitmapConstRef<float, 1> &src, int dx, int dy, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y) {
        byte *dstPixel = dst(dx, dy+y);
        for (int x = 0; x < w; ++x) {
            const float *srcPixel = src(sx+x, sy+y);
            *dstPixel++ = msdfgen::pixelFloatToByte(*srcPixel);
        }
    }
}

void blit(const msdfgen::BitmapRef<byte, 3> &dst, const msdfgen::BitmapConstRef<float, 3> &src, int dx, int dy, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y) {
        byte *dstPixel = dst(dx, dy+y);
        for (int x = 0; x < w; ++x) {
            const float *srcPixel = src(sx+x, sy+y);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[0]);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[1]);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[2]);
        }
    }
}

void blit(const msdfgen::BitmapRef<byte, 4> &dst, const msdfgen::BitmapConstRef<float, 4> &src, int dx, int dy, int sx, int sy, int w, int h) {
    for (int y = 0; y < h; ++y) {
        byte *dstPixel = dst(dx, dy+y);
        for (int x = 0; x < w; ++x) {
            const float *srcPixel = src(sx+x, sy+y);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[0]);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[1]);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[2]);
            *dstPixel++ = msdfgen::pixelFloatToByte(srcPixel[3]);
        }
    }
}

}
