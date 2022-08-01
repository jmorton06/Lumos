
#pragma once

namespace msdf_atlas {

/// The glyph box - its bounds in plane and atlas
struct GlyphBox {
    int index;
    double advance;
    struct {
        double l, b, r, t;
    } bounds;
    struct {
        int x, y, w, h;
    } rect;

};

}
