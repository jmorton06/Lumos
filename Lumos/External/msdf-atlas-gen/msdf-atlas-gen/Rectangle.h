
#pragma once

namespace msdf_atlas {

struct Rectangle {
    int x, y, w, h;
};

struct OrientedRectangle : Rectangle {
    bool rotated;
};

}
