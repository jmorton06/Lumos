
#include "DynamicAtlas.h"

namespace msdf_atlas {

template <class AtlasGenerator>
DynamicAtlas<AtlasGenerator>::DynamicAtlas() : glyphCount(0), side(0), totalArea(0), padding(0) { }

template <class AtlasGenerator>
DynamicAtlas<AtlasGenerator>::DynamicAtlas(AtlasGenerator &&generator) : generator((AtlasGenerator &&) generator), glyphCount(0), side(0), totalArea(0), padding(0) { }

template <class AtlasGenerator>
void DynamicAtlas<AtlasGenerator>::add(GlyphGeometry *glyphs, int count) {
    int start = rectangles.size();
    for (int i = 0; i < count; ++i) {
        if (!glyphs[i].isWhitespace()) {
            int w, h;
            glyphs[i].getBoxSize(w, h);
            Rectangle rect = { 0, 0, w+padding, h+padding };
            rectangles.push_back(rect);
            Remap remapEntry = { };
            remapEntry.index = glyphCount+i;
            remapEntry.width = w;
            remapEntry.height = h;
            remapBuffer.push_back(remapEntry);
            totalArea += (w+padding)*(h+padding);
        }
    }
    if ((int) rectangles.size() > start) {
        int oldSide = side;
        int packerStart = start;
        while (packer.pack(rectangles.data()+packerStart, rectangles.size()-packerStart) > 0) {
            side = side+!side<<1;
            while (side*side < totalArea)
                side <<= 1;
            packer = RectanglePacker(side+padding, side+padding);
            packerStart = 0;
        }
        if (packerStart < start) {
            for (int i = 0; i < start; ++i) {
                Remap &remap = remapBuffer[i];
                remap.source = remap.target;
                remap.target.x = rectangles[i].x;
                remap.target.y = rectangles[i].y;
            }
            generator.rearrange(side, side, remapBuffer.data(), start);
        } else if (side != oldSide)
            generator.resize(side, side);
        for (int i = start; i < (int) rectangles.size(); ++i) {
            remapBuffer[i].target.x = rectangles[i].x;
            remapBuffer[i].target.y = rectangles[i].y;
            glyphs[remapBuffer[i].index-glyphCount].placeBox(rectangles[i].x, rectangles[i].y);
        }
    }
    generator.generate(glyphs, count, genAttribs);
    glyphCount += count;
}

template <class AtlasGenerator>
AtlasGenerator & DynamicAtlas<AtlasGenerator>::atlasGenerator() {
    return generator;
}

template <class AtlasGenerator>
const AtlasGenerator & DynamicAtlas<AtlasGenerator>::atlasGenerator() const {
    return generator;
}

}
