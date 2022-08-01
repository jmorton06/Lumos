
#include "glyph-generators.h"

namespace msdf_atlas {

void scanlineGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs) {
    msdfgen::rasterize(output, glyph.getShape(), glyph.getBoxScale(), glyph.getBoxTranslate(), MSDF_ATLAS_GLYPH_FILL_RULE);
}

void sdfGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs) {
    msdfgen::generateSDF(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), attribs.config);
    if (attribs.scanlinePass)
        msdfgen::distanceSignCorrection(output, glyph.getShape(), glyph.getBoxProjection(), MSDF_ATLAS_GLYPH_FILL_RULE);
}

void psdfGenerator(const msdfgen::BitmapRef<float, 1> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs) {
    msdfgen::generatePseudoSDF(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), attribs.config);
    if (attribs.scanlinePass)
        msdfgen::distanceSignCorrection(output, glyph.getShape(), glyph.getBoxProjection(), MSDF_ATLAS_GLYPH_FILL_RULE);
}

void msdfGenerator(const msdfgen::BitmapRef<float, 3> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs) {
    msdfgen::MSDFGeneratorConfig config = attribs.config;
    if (attribs.scanlinePass)
        config.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::DISABLED;
    msdfgen::generateMSDF(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), config);
    if (attribs.scanlinePass) {
        msdfgen::distanceSignCorrection(output, glyph.getShape(), glyph.getBoxProjection(), MSDF_ATLAS_GLYPH_FILL_RULE);
        if (attribs.config.errorCorrection.mode != msdfgen::ErrorCorrectionConfig::DISABLED) {
            config.errorCorrection.mode = attribs.config.errorCorrection.mode;
            config.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            msdfgen::msdfErrorCorrection(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), config);
        }
    }
}

void mtsdfGenerator(const msdfgen::BitmapRef<float, 4> &output, const GlyphGeometry &glyph, const GeneratorAttributes &attribs) {
    msdfgen::MSDFGeneratorConfig config = attribs.config;
    if (attribs.scanlinePass)
        config.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::DISABLED;
    msdfgen::generateMTSDF(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), config);
    if (attribs.scanlinePass) {
        msdfgen::distanceSignCorrection(output, glyph.getShape(), glyph.getBoxProjection(), MSDF_ATLAS_GLYPH_FILL_RULE);
        if (attribs.config.errorCorrection.mode != msdfgen::ErrorCorrectionConfig::DISABLED) {
            config.errorCorrection.mode = attribs.config.errorCorrection.mode;
            config.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DO_NOT_CHECK_DISTANCE;
            msdfgen::msdfErrorCorrection(output, glyph.getShape(), glyph.getBoxProjection(), glyph.getBoxRange(), config);
        }
    }
}

}
