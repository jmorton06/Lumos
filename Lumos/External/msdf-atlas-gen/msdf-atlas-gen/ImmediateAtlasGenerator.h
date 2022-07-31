
#pragma once

#include <vector>
#include "GlyphBox.h"
#include "Workload.h"
#include "AtlasGenerator.h"

namespace msdf_atlas {

/**
 * An implementation of AtlasGenerator that uses the specified generator function
 * and AtlasStorage class and generates glyph bitmaps immediately
 * (does not return until all submitted work is finished),
 * but may use multiple threads (setThreadCount).
 */
template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
class ImmediateAtlasGenerator {

public:
    ImmediateAtlasGenerator();
    ImmediateAtlasGenerator(int width, int height);
    void generate(const GlyphGeometry *glyphs, int count);
    void rearrange(int width, int height, const Remap *remapping, int count);
    void resize(int width, int height);
    /// Sets attributes for the generator function
    void setAttributes(const GeneratorAttributes &attributes);
    /// Sets the number of threads to be run by generate
    void setThreadCount(int threadCount);
    /// Allows access to the underlying AtlasStorage
    const AtlasStorage & atlasStorage() const;

private:
    AtlasStorage storage;
    std::vector<GlyphBox> layout;
    std::vector<T> glyphBuffer;
    std::vector<byte> errorCorrectionBuffer;
    GeneratorAttributes attributes;
    int threadCount;

};

}

#include "ImmediateAtlasGenerator.hpp"
