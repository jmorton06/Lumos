
#include "image-save.h"

#include <cstdio>
#include <msdfgen-ext.h>

namespace msdf_atlas {

template <int N>
bool saveImageBinary(const msdfgen::BitmapConstRef<byte, N> &bitmap, const char *filename, YDirection outputYDirection);
template <int N>
bool saveImageBinaryLE(const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection);
template <int N>
bool saveImageBinaryBE(const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection);

template <int N>
bool saveImageText(const msdfgen::BitmapConstRef<byte, N> &bitmap, const char *filename, YDirection outputYDirection);
template <int N>
bool saveImageText(const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection);

template <int N>
bool saveImage(const msdfgen::BitmapConstRef<byte, N> &bitmap, ImageFormat format, const char *filename, YDirection outputYDirection) {
    switch (format) {
        case ImageFormat::PNG:
            return msdfgen::savePng(bitmap, filename);
        case ImageFormat::BMP:
            return msdfgen::saveBmp(bitmap, filename);
        case ImageFormat::TIFF:
            return false;
        case ImageFormat::TEXT:
            return saveImageText(bitmap, filename, outputYDirection);
        case ImageFormat::TEXT_FLOAT:
            return false;
        case ImageFormat::BINARY:
            return saveImageBinary(bitmap, filename, outputYDirection);
        case ImageFormat::BINARY_FLOAT:
        case ImageFormat::BINARY_FLOAT_BE:
            return false;
        default:;
    }
    return false;
}

template <int N>
bool saveImage(const msdfgen::BitmapConstRef<float, N> &bitmap, ImageFormat format, const char *filename, YDirection outputYDirection) {
    switch (format) {
        case ImageFormat::PNG:
            return msdfgen::savePng(bitmap, filename);
        case ImageFormat::BMP:
            return msdfgen::saveBmp(bitmap, filename);
        case ImageFormat::TIFF:
            return msdfgen::saveTiff(bitmap, filename);
        case ImageFormat::TEXT:
            return false;
        case ImageFormat::TEXT_FLOAT:
            return saveImageText(bitmap, filename, outputYDirection);
        case ImageFormat::BINARY:
            return false;
        case ImageFormat::BINARY_FLOAT:
            return saveImageBinaryLE(bitmap, filename, outputYDirection);
        case ImageFormat::BINARY_FLOAT_BE:
            return saveImageBinaryBE(bitmap, filename, outputYDirection);
        default:;
    }
    return false;
}

template <int N>
bool saveImageBinary(const msdfgen::BitmapConstRef<byte, N> &bitmap, const char *filename, YDirection outputYDirection) {
    bool success = false;
    if (FILE *f = fopen(filename, "wb")) {
        int written = 0;
        switch (outputYDirection) {
            case YDirection::BOTTOM_UP:
                written = fwrite(bitmap.pixels, 1, N*bitmap.width*bitmap.height, f);
                break;
            case YDirection::TOP_DOWN:
                for (int y = bitmap.height-1; y >= 0; --y)
                    written += fwrite(bitmap.pixels+N*bitmap.width*y, 1, N*bitmap.width, f);
                break;
        }
        success = written == N*bitmap.width*bitmap.height;
        fclose(f);
    }
    return success;
}

template <int N>
bool
    #ifdef __BIG_ENDIAN__
        saveImageBinaryBE
    #else
        saveImageBinaryLE
    #endif
        (const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection) {
    bool success = false;
    if (FILE *f = fopen(filename, "wb")) {
        int written = 0;
        switch (outputYDirection) {
            case YDirection::BOTTOM_UP:
                written = fwrite(bitmap.pixels, sizeof(float), N*bitmap.width*bitmap.height, f);
                break;
            case YDirection::TOP_DOWN:
                for (int y = bitmap.height-1; y >= 0; --y)
                    written += fwrite(bitmap.pixels+N*bitmap.width*y, sizeof(float), N*bitmap.width, f);
                break;
        }
        success = written == N*bitmap.width*bitmap.height;
        fclose(f);
    }
    return success;
}

template <int N>
bool
    #ifdef __BIG_ENDIAN__
        saveImageBinaryLE
    #else
        saveImageBinaryBE
    #endif
        (const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection) {
    bool success = false;
    if (FILE *f = fopen(filename, "wb")) {
        int written = 0;
        for (int y = 0; y < bitmap.height; ++y) {
            const float *p = bitmap.pixels+N*bitmap.width*(outputYDirection == YDirection::TOP_DOWN ? bitmap.height-y-1 : y);
            for (int x = 0; x < bitmap.width; ++x) {
                const unsigned char *b = reinterpret_cast<const unsigned char *>(p++);
                for (int i = sizeof(float)-1; i >= 0; --i)
                    written += fwrite(b+i, 1, 1, f);
            }
        }
        success = written == sizeof(float)*N*bitmap.width*bitmap.height;
        fclose(f);
    }
    return success;
}


template <int N>
bool saveImageText(const msdfgen::BitmapConstRef<byte, N> &bitmap, const char *filename, YDirection outputYDirection) {
    bool success = false;
    if (FILE *f = fopen(filename, "wb")) {
        success = true;
        for (int y = 0; y < bitmap.height; ++y) {
            const byte *p = bitmap.pixels+N*bitmap.width*(outputYDirection == YDirection::TOP_DOWN ? bitmap.height-y-1 : y);
            for (int x = 0; x < N*bitmap.width; ++x)
                success &= fprintf(f, x ? " %02X" : "%02X", (unsigned) *p++) > 0;
            success &= fprintf(f, "\n") > 0;
        }
        fclose(f);
    }
    return success;
}

template <int N>
bool saveImageText(const msdfgen::BitmapConstRef<float, N> &bitmap, const char *filename, YDirection outputYDirection) {
    bool success = false;
    if (FILE *f = fopen(filename, "wb")) {
        success = true;
        for (int y = 0; y < bitmap.height; ++y) {
            const float *p = bitmap.pixels+N*bitmap.width*(outputYDirection == YDirection::TOP_DOWN ? bitmap.height-y-1 : y);
            for (int x = 0; x < N*bitmap.width; ++x)
                success &= fprintf(f, x ? " %g" : "%g", *p++) > 0;
            success &= fprintf(f, "\n") > 0;
        }
        fclose(f);
    }
    return success;
}

}
