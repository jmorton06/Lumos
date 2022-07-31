
#include "image-encode.h"

#include <lodepng.h>

namespace msdf_atlas {

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 1> &bitmap) {
    std::vector<byte> pixels(bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[bitmap.width*y], bitmap(0, bitmap.height-y-1), bitmap.width);
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_GREY);
}

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 3> &bitmap) {
    std::vector<byte> pixels(3*bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[3*bitmap.width*y], bitmap(0, bitmap.height-y-1), 3*bitmap.width);
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_RGB);
}

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 4> &bitmap) {
    std::vector<byte> pixels(4*bitmap.width*bitmap.height);
    for (int y = 0; y < bitmap.height; ++y)
        memcpy(&pixels[4*bitmap.width*y], bitmap(0, bitmap.height-y-1), 4*bitmap.width);
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_RGBA);
}

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 1> &bitmap) {
    std::vector<byte> pixels(bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x)
            *it++ = msdfgen::pixelFloatToByte(*bitmap(x, y));
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_GREY);
}

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 3> &bitmap) {
    std::vector<byte> pixels(3*bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x) {
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[1]);
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[2]);
        }
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_RGB);
}

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 4> &bitmap) {
    std::vector<byte> pixels(4*bitmap.width*bitmap.height);
    std::vector<byte>::iterator it = pixels.begin();
    for (int y = bitmap.height-1; y >= 0; --y)
        for (int x = 0; x < bitmap.width; ++x) {
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[1]);
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[2]);
            *it++ = msdfgen::pixelFloatToByte(bitmap(x, y)[3]);
        }
    return !lodepng::encode(output, pixels, bitmap.width, bitmap.height, LCT_RGBA);
}

}
