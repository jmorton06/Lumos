
#pragma once

namespace msdf_atlas {

/// Represents the repositioning of a subsection of the atlas
struct Remap {
    int index;
    struct {
        int x, y;
    } source, target;
    int width, height;
};

}
