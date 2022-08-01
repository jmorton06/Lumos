
#pragma once

#include <msdfgen.h>
#include "Remap.h"

namespace msdf_atlas {

namespace {

/** Prototype of an atlas storage class.
 *  An atlas storage physically holds the pixels of the atlas
 *  and allows to read and write subsections represented as bitmaps.
 *  Can be implemented using a simple bitmap (BitmapAtlasStorage),
 *  as texture memory, or any other way.
 */
class AtlasStorage {

public:
    AtlasStorage();
    AtlasStorage(int width, int height);
    /// Creates a copy with different dimensions
    AtlasStorage(const AtlasStorage &orig, int width, int height);
    /// Creates a copy with different dimensions and rearranges the pixels according to the remapping array
    AtlasStorage(const AtlasStorage &orig, int width, int height, const Remap *remapping, int count);
    /// Stores a subsection at x, y into the atlas storage. May be implemented for only some T, N
    template <typename T, int N>
    void put(int x, int y, const msdfgen::BitmapConstRef<T, N> &subBitmap);
    /// Retrieves a subsection at x, y from the atlas storage. May be implemented for only some T, N
    template <typename T, int N>
    void get(int x, int y, const msdfgen::BitmapRef<T, N> &subBitmap) const;

};

}

}
