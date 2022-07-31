
#pragma once

#include "FontGeometry.h"

namespace msdf_atlas {

/**
 * Writes the positioning data and atlas layout of the glyphs into a CSV file
 * The columns are: font variant index (if fontCount > 1), glyph identifier (index or Unicode), horizontal advance, plane bounds (l, b, r, t), atlas bounds (l, b, r, t)
 */
bool exportCSV(const FontGeometry *fonts, int fontCount, int atlasWidth, int atlasHeight, YDirection yDirection, const char *filename);

}
