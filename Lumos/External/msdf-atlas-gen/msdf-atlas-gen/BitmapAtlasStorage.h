
#pragma once

#include "AtlasStorage.h"

namespace msdf_atlas {

/// An implementation of AtlasStorage represented by a bitmap in memory (msdfgen::Bitmap)
template <typename T, int N>
class BitmapAtlasStorage {

public:
    BitmapAtlasStorage();
    BitmapAtlasStorage(int width, int height);
    explicit BitmapAtlasStorage(const msdfgen::BitmapConstRef<T, N> &bitmap);
    explicit BitmapAtlasStorage(msdfgen::Bitmap<T, N> &&bitmap);
    BitmapAtlasStorage(const BitmapAtlasStorage<T, N> &orig, int width, int height);
    BitmapAtlasStorage(const BitmapAtlasStorage<T, N> &orig, int width, int height, const Remap *remapping, int count);
    operator msdfgen::BitmapConstRef<T, N>() const;
    operator msdfgen::BitmapRef<T, N>();
    operator msdfgen::Bitmap<T, N>() &&;
    template <typename S>
    void put(int x, int y, const msdfgen::BitmapConstRef<S, N> &subBitmap);
    void get(int x, int y, const msdfgen::BitmapRef<T, N> &subBitmap) const;

private:
    msdfgen::Bitmap<T, N> bitmap;

};

}

#include "BitmapAtlasStorage.hpp"
