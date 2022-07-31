
#pragma once

#include <utility>
#include <vector>
#include <string>
#include <map>
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include "types.h"
#include "GlyphGeometry.h"
#include "Charset.h"

#define MSDF_ATLAS_DEFAULT_EM_SIZE 32.0

namespace msdf_atlas {

/// Represents the geometry of all glyphs of a given font or font variant
class FontGeometry {

public:
    class GlyphRange {
    public:
        GlyphRange();
        GlyphRange(const std::vector<GlyphGeometry> *glyphs, size_t rangeStart, size_t rangeEnd);
        size_t size() const;
        bool empty() const;
        const GlyphGeometry * begin() const;
        const GlyphGeometry * end() const;
    private:
        const std::vector<GlyphGeometry> *glyphs;
        size_t rangeStart, rangeEnd;
    };

    FontGeometry();
    explicit FontGeometry(std::vector<GlyphGeometry> *glyphStorage);

    /// Loads all glyphs in a glyphset (Charset elements are glyph indices), returns the number of successfully loaded glyphs
    int loadGlyphset(msdfgen::FontHandle *font, double fontScale, const Charset &glyphset, bool preprocessGeometry = true, bool enableKerning = true);
    /// Loads all glyphs in a charset (Charset elements are Unicode codepoints), returns the number of successfully loaded glyphs
    int loadCharset(msdfgen::FontHandle *font, double fontScale, const Charset &charset, bool preprocessGeometry = true, bool enableKerning = true);

    /// Only loads font metrics and geometry scale from font
    bool loadMetrics(msdfgen::FontHandle *font, double fontScale);
    /// Adds a loaded glyph
    bool addGlyph(const GlyphGeometry &glyph);
    bool addGlyph(GlyphGeometry &&glyph);
    /// Loads kerning pairs for all glyphs that are currently present, returns the number of loaded kerning pairs
    int loadKerning(msdfgen::FontHandle *font);
    /// Sets a name to be associated with the font
    void setName(const char *name);

    /// Returns the geometry scale to be used when loading glyphs
    double getGeometryScale() const;
    /// Returns the processed font metrics
    const msdfgen::FontMetrics & getMetrics() const;
    /// Returns the type of identifier that was used to load glyphs
    GlyphIdentifierType getPreferredIdentifierType() const;
    /// Returns the list of all glyphs
    GlyphRange getGlyphs() const;
    /// Finds a glyph by glyph index or Unicode codepoint, returns null if not found
    const GlyphGeometry * getGlyph(msdfgen::GlyphIndex index) const;
    const GlyphGeometry * getGlyph(unicode_t codepoint) const;
    /// Outputs the advance between two glyphs with kerning taken into consideration, returns false on failure
    bool getAdvance(double &advance, msdfgen::GlyphIndex index1, msdfgen::GlyphIndex index2) const;
    bool getAdvance(double &advance, unicode_t codepoint1, unicode_t codepoint2) const;
    /// Returns the complete mapping of kerning pairs (by glyph indices) and their respective advance values
    const std::map<std::pair<int, int>, double> & getKerning() const;
    /// Returns the name associated with the font or null if not set
    const char * getName() const;

private:
    double geometryScale;
    msdfgen::FontMetrics metrics;
    GlyphIdentifierType preferredIdentifierType;
    std::vector<GlyphGeometry> *glyphs;
    size_t rangeStart, rangeEnd;
    std::map<int, size_t> glyphsByIndex;
    std::map<unicode_t, size_t> glyphsByCodepoint;
    std::map<std::pair<int, int>, double> kerning;
    std::vector<GlyphGeometry> ownGlyphs;
    std::string name;

};

}
