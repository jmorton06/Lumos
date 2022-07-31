
#include "rectangle-packing.h"

#include <vector>
#include "RectanglePacker.h"

namespace msdf_atlas {

static void copyRectanglePlacement(Rectangle &dst, const Rectangle &src) {
    dst.x = src.x;
    dst.y = src.y;
}

static void copyRectanglePlacement(OrientedRectangle &dst, const OrientedRectangle &src) {
    dst.x = src.x;
    dst.y = src.y;
    dst.rotated = src.rotated;
}

template <typename RectangleType>
int packRectangles(RectangleType *rectangles, int count, int width, int height, int padding) {
    if (padding)
        for (int i = 0; i < count; ++i) {
            rectangles[i].w += padding;
            rectangles[i].h += padding;
        }
    int result = RectanglePacker(width+padding, height+padding).pack(rectangles, count);
    if (padding)
        for (int i = 0; i < count; ++i) {
            rectangles[i].w -= padding;
            rectangles[i].h -= padding;
        }
    return result;
}

template <class SizeSelector, typename RectangleType>
std::pair<int, int> packRectangles(RectangleType *rectangles, int count, int padding) {
    std::vector<RectangleType> rectanglesCopy(count);
    int totalArea = 0;
    for (int i = 0; i < count; ++i) {
        rectanglesCopy[i].w = rectangles[i].w+padding;
        rectanglesCopy[i].h = rectangles[i].h+padding;
        totalArea += rectangles[i].w*rectangles[i].h;
    }
    std::pair<int, int> dimensions;
    SizeSelector sizeSelector(totalArea);
    int width, height;
    while (sizeSelector(width, height)) {
        if (!RectanglePacker(width+padding, height+padding).pack(rectanglesCopy.data(), count)) {
            dimensions.first = width;
            dimensions.second = height;
            for (int i = 0; i < count; ++i)
                copyRectanglePlacement(rectangles[i], rectanglesCopy[i]);
            --sizeSelector;
        } else
            ++sizeSelector;
    }
    return dimensions;
}

}
