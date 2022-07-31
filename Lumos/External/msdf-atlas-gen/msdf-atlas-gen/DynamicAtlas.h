
#pragma once

#include <vector>
#include "RectanglePacker.h"
#include "AtlasGenerator.h"

namespace msdf_atlas {

/**
 * This class can be used to produce a dynamic atlas to which more glyphs are added over time.
 * It takes care of laying out and enlarging the atlas as necessary and delegates the actual work
 * to the specified AtlasGenerator, which may e.g. do the work asynchronously.
 */
template <class AtlasGenerator>
class DynamicAtlas {

public:
    DynamicAtlas();
    /// Creates with a configured generator. The generator must not contain any prior glyphs!
    explicit DynamicAtlas(AtlasGenerator &&generator);
    /// Adds a batch of glyphs. Adding more than one glyph at a time may improve packing efficiency
    void add(GlyphGeometry *glyphs, int count);
    /// Allows access to generator. Do not add glyphs to the generator directly!
    AtlasGenerator & atlasGenerator();
    const AtlasGenerator & atlasGenerator() const;

private:
    AtlasGenerator generator;
    RectanglePacker packer;
    int glyphCount;
    int side;
    std::vector<Rectangle> rectangles;
    std::vector<Remap> remapBuffer;
    int totalArea;
    GeneratorAttributes genAttribs;
    int padding;

};

}

#include "DynamicAtlas.hpp"
