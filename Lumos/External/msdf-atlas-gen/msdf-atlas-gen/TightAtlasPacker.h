
#pragma once

#include "GlyphGeometry.h"

namespace msdf_atlas {

/**
 * This class computes the layout of a static atlas and may optionally
 * also find the minimum required dimensions and/or the maximum glyph scale
 */
class TightAtlasPacker {

public:
    /// Constraints for the atlas's dimensions - see size selectors for more info
    enum class DimensionsConstraint {
        POWER_OF_TWO_SQUARE,
        POWER_OF_TWO_RECTANGLE,
        MULTIPLE_OF_FOUR_SQUARE,
        EVEN_SQUARE,
        SQUARE
    };

    TightAtlasPacker();

    /// Computes the layout for the array of glyphs. Returns 0 on success
    int pack(GlyphGeometry *glyphs, int count);

    /// Sets the atlas's dimensions to be fixed
    void setDimensions(int width, int height);
    /// Sets the atlas's dimensions to be determined during pack
    void unsetDimensions();
    /// Sets the constraint to be used when determining dimensions
    void setDimensionsConstraint(DimensionsConstraint dimensionsConstraint);
    /// Sets the padding between glyph boxes
    void setPadding(int padding);
    /// Sets fixed glyph scale
    void setScale(double scale);
    /// Sets the minimum glyph scale
    void setMinimumScale(double minScale);
    /// Sets the unit component of the total distance range
    void setUnitRange(double unitRange);
    /// Sets the pixel component of the total distance range
    void setPixelRange(double pxRange);
    /// Sets the miter limit for bounds computation
    void setMiterLimit(double miterLimit);

    /// Outputs the atlas's final dimensions
    void getDimensions(int &width, int &height) const;
    /// Returns the final glyph scale
    double getScale() const;
    /// Returns the final combined pixel range (including converted unit range)
    double getPixelRange() const;

private:
    int width, height;
    int padding;
    DimensionsConstraint dimensionsConstraint;
    double scale;
    double minScale;
    double unitRange;
    double pxRange;
    double miterLimit;
    double scaleMaximizationTolerance;

    static int tryPack(GlyphGeometry *glyphs, int count, DimensionsConstraint dimensionsConstraint, int &width, int &height, int padding, double scale, double range, double miterLimit);
    static double packAndScale(GlyphGeometry *glyphs, int count, int width, int height, int padding, double unitRange, double pxRange, double miterLimit, double tolerance);

};

}
