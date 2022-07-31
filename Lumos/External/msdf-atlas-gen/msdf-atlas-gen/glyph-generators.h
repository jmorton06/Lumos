
#pragma once

#include <msdfgen.h>
#include "GlyphGeometry.h"
#include "AtlasGenerator.h"

#define MSDF_ATLAS_GLYPH_FILL_RULE msdfgen::FILL_NONZERO

namespace msdf_atlas {

// Glyph bitmap generator functions

/// Generates non-anti-aliased binary image of the glyph using scanline rasterization
void scanlineGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs);
/// Generates a true signed distance field of the glyph
void sdfGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs);
/// Generates a signed pseudo-distance field of the glyph
void psdfGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs);
/// Generates a multi-channel signed distance field of the glyph
void msdfGenerator(const msdfgen::BitmapRef<float, 3> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs);
/// Generates a multi-channel and alpha-encoded true signed distance field of the glyph
void mtsdfGenerator(const msdfgen::BitmapRef<float, 4> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs);

}
