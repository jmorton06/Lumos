
#pragma once

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include "types.h"
#include "FontGeometry.h"

namespace msdf_atlas {

/// Writes the font and glyph metrics and atlas layout data into a comprehensive JSON file
bool exportJSON(const FontGeometry *fonts, int fontCount, double fontSize, double pxRange, int atlasWidth, int atlasHeight, ImageType imageType, YDirection yDirection, const char *filename, bool kerning);

}
