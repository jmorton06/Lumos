
#include "BitmapAtlasStorage.h"

#include <cstring>
#include <algorithm>
#include "bitmap-blit.h"

namespace msdf_atlas {

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage() { }

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage(int width, int height) : bitmap(width, height) {
    memset((T *) bitmap, 0, sizeof(T)*N*width*height);
}

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage(const msdfgen::BitmapConstRef<T, N> &bitmap) : bitmap(bitmap) { }

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage(msdfgen::Bitmap<T, N> &&bitmap) : bitmap((msdfgen::Bitmap<T, N> &&) bitmap) { }

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage(const BitmapAtlasStorage<T, N> &orig, int width, int height) : bitmap(width, height) {
    memset((T *) bitmap, 0, sizeof(T)*N*width*height);
    blit(bitmap, orig.bitmap, 0, 0, 0, 0, std::min(width, orig.bitmap.width()), std::min(height, orig.bitmap.height()));
}

template <typename T, int N>
BitmapAtlasStorage<T, N>::BitmapAtlasStorage(const BitmapAtlasStorage<T, N> &orig, int width, int height, const Remap *remapping, int count) : bitmap(width, height) {
    memset((T *) bitmap, 0, sizeof(T)*N*width*height);
    for (int i = 0; i < count; ++i) {
        const Remap &remap = remapping[i];
        blit(bitmap, orig.bitmap, remap.target.x, remap.target.y, remap.source.x, remap.source.y, remap.width, remap.height);
    }
}

template <typename T, int N>
BitmapAtlasStorage<T, N>::operator msdfgen::BitmapConstRef<T, N>() const {
    return bitmap;
}

template <typename T, int N>
BitmapAtlasStorage<T, N>::operator msdfgen::BitmapRef<T, N>() {
    return bitmap;
}

template <typename T, int N>
BitmapAtlasStorage<T, N>::operator msdfgen::Bitmap<T, N>() && {
    return (msdfgen::Bitmap<T, N> &&) bitmap;
}

template <typename T, int N>
template <typename S>
void BitmapAtlasStorage<T, N>::put(int x, int y, const msdfgen::BitmapConstRef<S, N> &subBitmap) {
    blit(bitmap, subBitmap, x, y, 0, 0, subBitmap.width, subBitmap.height);
}

template <typename T, int N>
void BitmapAtlasStorage<T, N>::get(int x, int y, const msdfgen::BitmapRef<T, N> &subBitmap) const {
    blit(subBitmap, bitmap, 0, 0, x, y, subBitmap.width, subBitmap.height);
}

}
