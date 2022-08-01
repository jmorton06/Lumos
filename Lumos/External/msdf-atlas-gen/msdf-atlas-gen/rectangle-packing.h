
#pragma once

#include <utility>
#include "Rectangle.h"

namespace msdf_atlas {

/// Packs the rectangle array into an atlas with fixed dimensions, returns how many didn't fit (0 on success)
template <typename RectangleType>
int packRectangles(RectangleType *rectangles, int count, int width, int height, int padding = 0);

/// Packs the rectangle array into an atlas of unknown size, returns the minimum required dimensions constrained by SizeSelector
template <class SizeSelector, typename RectangleType>
std::pair<int, int> packRectangles(RectangleType *rectangles, int count, int padding = 0);

}

#include "rectangle-packing.hpp"
