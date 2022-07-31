
#include "csv-export.h"

#include <cstdio>
#include "GlyphGeometry.h"

namespace msdf_atlas {

bool exportCSV(const FontGeometry *fonts, int fontCount, int atlasWidth, int atlasHeight, YDirection yDirection, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f)
        return false;

    for (int i = 0; i < fontCount; ++i) {
        for (const GlyphGeometry &glyph : fonts[i].getGlyphs()) {
            double l, b, r, t;
            if (fontCount > 1)
                fprintf(f, "%d,", i);
            fprintf(f, "%d,%.17g,", glyph.getIdentifier(fonts[i].getPreferredIdentifierType()), glyph.getAdvance());
            glyph.getQuadPlaneBounds(l, b, r, t);
            switch (yDirection) {
                case YDirection::BOTTOM_UP:
                    fprintf(f, "%.17g,%.17g,%.17g,%.17g,", l, b, r, t);
                    break;
                case YDirection::TOP_DOWN:
                    fprintf(f, "%.17g,%.17g,%.17g,%.17g,", l, -t, r, -b);
                    break;
            }
            glyph.getQuadAtlasBounds(l, b, r, t);
            switch (yDirection) {
                case YDirection::BOTTOM_UP:
                    fprintf(f, "%.17g,%.17g,%.17g,%.17g\n", l, b, r, t);
                    break;
                case YDirection::TOP_DOWN:
                    fprintf(f, "%.17g,%.17g,%.17g,%.17g\n", l, atlasHeight-t, r, atlasHeight-b);
                    break;
            }
        }
    }

    fclose(f);
    return true;
}

}
