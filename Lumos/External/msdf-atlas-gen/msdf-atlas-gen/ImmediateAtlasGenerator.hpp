
#include "ImmediateAtlasGenerator.h"

#include <algorithm>

namespace msdf_atlas {

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::ImmediateAtlasGenerator() : threadCount(1) { }

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::ImmediateAtlasGenerator(int width, int height) : storage(width, height), threadCount(1) { }

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
void ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::generate(const GlyphGeometry *glyphs, int count) {
    int maxBoxArea = 0;
    for (int i = 0; i < count; ++i) {
        GlyphBox box = glyphs[i];
        maxBoxArea = std::max(maxBoxArea, box.rect.w*box.rect.h);
        layout.push_back((GlyphBox &&) box);
    }
    int threadBufferSize = N*maxBoxArea;
    if (threadCount*threadBufferSize > (int) glyphBuffer.size())
        glyphBuffer.resize(threadCount*threadBufferSize);
    if (threadCount*maxBoxArea > (int) errorCorrectionBuffer.size())
        errorCorrectionBuffer.resize(threadCount*maxBoxArea);
    std::vector<GeneratorAttributes> threadAttributes(threadCount);
    for (int i = 0; i < threadCount; ++i) {
        threadAttributes[i] = attributes;
        threadAttributes[i].config.errorCorrection.buffer = errorCorrectionBuffer.data()+i*maxBoxArea;
    }

    Workload([this, glyphs, &threadAttributes, threadBufferSize](int i, int threadNo) -> bool {
        const GlyphGeometry &glyph = glyphs[i];
        if (!glyph.isWhitespace()) {
            int l, b, w, h;
            glyph.getBoxRect(l, b, w, h);
            msdfgen::BitmapRef<T, N> glyphBitmap(glyphBuffer.data()+threadNo*threadBufferSize, w, h);
            GEN_FN(glyphBitmap, glyph, threadAttributes[threadNo]);
            storage.put(l, b, msdfgen::BitmapConstRef<T, N>(glyphBitmap));
        }
        return true;
    }, count).finish(threadCount);
}

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
void ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::rearrange(int width, int height, const Remap *remapping, int count) {
    for (int i = 0; i < count; ++i) {
        layout[remapping[i].index].rect.x = remapping[i].target.x;
        layout[remapping[i].index].rect.y = remapping[i].target.y;
    }
    AtlasStorage newStorage((AtlasStorage &&) storage, width, height, remapping, count);
    storage = (AtlasStorage &&) newStorage;
}

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
void ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::resize(int width, int height) {
    AtlasStorage newStorage((AtlasStorage &&) storage, width, height);
    storage = (AtlasStorage &&) newStorage;
}

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
void ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::setAttributes(const GeneratorAttributes &attributes) {
    this->attributes = attributes;
}

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
void ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::setThreadCount(int threadCount) {
    this->threadCount = threadCount;
}

template <typename T, int N, GeneratorFunction<T, N> GEN_FN, class AtlasStorage>
const AtlasStorage & ImmediateAtlasGenerator<T, N, GEN_FN, AtlasStorage>::atlasStorage() const {
    return storage;
}

}
