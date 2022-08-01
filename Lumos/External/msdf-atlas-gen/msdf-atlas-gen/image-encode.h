
#pragma once

#include <vector>
#include <msdfgen.h>
#include "types.h"

namespace msdf_atlas {

// Functions to encode an image as a sequence of bytes in memory
// Only PNG format available currently

bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 1> &bitmap);
bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 3> &bitmap);
bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<msdfgen::byte, 4> &bitmap);
bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 1> &bitmap);
bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 3> &bitmap);
bool encodePng(std::vector<byte> &output, const msdfgen::BitmapConstRef<float, 4> &bitmap);

}
