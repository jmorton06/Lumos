#pragma once
#undef INFINITE
#include <msdf-atlas-gen.h>
#include <vector>

namespace Lumos
{
    struct MSDFData
    {
        msdf_atlas::FontGeometry FontGeometry;
        msdf_atlas::FontGeometry IconFontGeometry;
        std::vector<msdf_atlas::GlyphGeometry> Glyphs;

        // Resolve a codepoint against the text font first, then the icon font.
        const msdf_atlas::GlyphGeometry* GetGlyph(uint32_t codepoint) const
        {
            // no-break / thin / narrow spaces (price and number formatting) draw as a plain space
            if(codepoint == 0x00A0 || (codepoint >= 0x2000 && codepoint <= 0x200A) || codepoint == 0x202F || codepoint == 0x205F || codepoint == 0x3000)
                codepoint = ' ';
            if(const auto* g = FontGeometry.getGlyph(msdfgen::unicode_t(codepoint)))
                return g;
            return IconFontGeometry.getGlyph(msdfgen::unicode_t(codepoint));
        }
    };
}
