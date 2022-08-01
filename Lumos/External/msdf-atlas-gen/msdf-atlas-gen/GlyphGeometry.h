
#pragma once

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include "types.h"
#include "GlyphBox.h"

namespace msdf_atlas {

/// Represents the shape geometry of a single glyph as well as its configuration
class GlyphGeometry {

public:
    GlyphGeometry();
    /// Loads glyph geometry from font
    bool load(msdfgen::FontHandle *font, double geometryScale, msdfgen::GlyphIndex index, bool preprocessGeometry = true);
    bool load(msdfgen::FontHandle *font, double geometryScale, unicode_t codepoint, bool preprocessGeometry = true);
    /// Applies edge coloring to glyph shape
    void edgeColoring(void (*fn)(msdfgen::Shape &, double, unsigned long long), double angleThreshold, unsigned long long seed);
    /// Computes the dimensions of the glyph's box as well as the transformation for the generator function
    void wrapBox(double scale, double range, double miterLimit);
    /// Sets the glyph's box's position in the atlas
    void placeBox(int x, int y);
    /// Returns the glyph's index within the font
    int getIndex() const;
    /// Returns the glyph's index as a msdfgen::GlyphIndex
    msdfgen::GlyphIndex getGlyphIndex() const;
    /// Returns the Unicode codepoint represented by the glyph or 0 if unknown
    unicode_t getCodepoint() const;
    /// Returns the glyph's identifier specified by the supplied identifier type
    int getIdentifier(GlyphIdentifierType type) const;
    /// Returns the glyph's shape
    const msdfgen::Shape & getShape() const;
    /// Returns the glyph's advance
    double getAdvance() const;
    /// Outputs the position and dimensions of the glyph's box in the atlas
    void getBoxRect(int &x, int &y, int &w, int &h) const;
    /// Outputs the dimensions of the glyph's box in the atlas
    void getBoxSize(int &w, int &h) const;
    /// Returns the range needed to generate the glyph's SDF
    double getBoxRange() const;
    /// Returns the projection needed to generate the glyph's bitmap
    msdfgen::Projection getBoxProjection() const;
    /// Returns the scale needed to generate the glyph's bitmap
    double getBoxScale() const;
    /// Returns the translation vector needed to generate the glyph's bitmap
    msdfgen::Vector2 getBoxTranslate() const;
    /// Outputs the bounding box of the glyph as it should be placed on the baseline
    void getQuadPlaneBounds(double &l, double &b, double &r, double &t) const;
    /// Outputs the bounding box of the glyph in the atlas
    void getQuadAtlasBounds(double &l, double &b, double &r, double &t) const;
    /// Returns true if the glyph is a whitespace and has no geometry
    bool isWhitespace() const;
    /// Simplifies to GlyphBox
    operator GlyphBox() const;

private:
    int index;
    unicode_t codepoint;
    double geometryScale;
    msdfgen::Shape shape;
    msdfgen::Shape::Bounds bounds;
    double advance;
    struct {
        struct {
            int x, y, w, h;
        } rect;
        double range;
        double scale;
        msdfgen::Vector2 translate;
    } box;

};

}
