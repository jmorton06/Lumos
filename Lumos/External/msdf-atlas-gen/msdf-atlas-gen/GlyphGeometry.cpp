
#include "GlyphGeometry.h"

#include <cmath>
#include <core/ShapeDistanceFinder.h>

namespace msdf_atlas {

GlyphGeometry::GlyphGeometry() : index(), codepoint(), geometryScale(), bounds(), advance(), box() { }

bool GlyphGeometry::load(msdfgen::FontHandle *font, double geometryScale, msdfgen::GlyphIndex index, bool preprocessGeometry) {
    if (font && msdfgen::loadGlyph(shape, font, index, &advance) && shape.validate()) {
        this->index = index.getIndex();
        this->geometryScale = geometryScale;
        codepoint = 0;
        advance *= geometryScale;
        #ifdef MSDFGEN_USE_SKIA
            if (preprocessGeometry)
                msdfgen::resolveShapeGeometry(shape);
        #endif
        shape.normalize();
        bounds = shape.getBounds();
        #ifdef MSDFGEN_USE_SKIA
            if (!preprocessGeometry)
        #endif
        {
            // Determine if shape is winded incorrectly and reverse it in that case
            msdfgen::Point2 outerPoint(bounds.l-(bounds.r-bounds.l)-1, bounds.b-(bounds.t-bounds.b)-1);
            if (msdfgen::SimpleTrueShapeDistanceFinder::oneShotDistance(shape, outerPoint) > 0) {
                for (msdfgen::Contour &contour : shape.contours)
                    contour.reverse();
            }
        }
        return true;
    }
    return false;
}

bool GlyphGeometry::load(msdfgen::FontHandle *font, double geometryScale, unicode_t codepoint, bool preprocessGeometry) {
    msdfgen::GlyphIndex index;
    if (msdfgen::getGlyphIndex(index, font, codepoint)) {
        if (load(font, geometryScale, index, preprocessGeometry)) {
            this->codepoint = codepoint;
            return true;
        }
    }
    return false;
}

void GlyphGeometry::edgeColoring(void (*fn)(msdfgen::Shape &, double, unsigned long long), double angleThreshold, unsigned long long seed) {
    fn(shape, angleThreshold, seed);
}

void GlyphGeometry::wrapBox(double scale, double range, double miterLimit) {
    scale *= geometryScale;
    range /= geometryScale;
    box.range = range;
    box.scale = scale;
    if (bounds.l < bounds.r && bounds.b < bounds.t) {
        double l = bounds.l, b = bounds.b, r = bounds.r, t = bounds.t;
        l -= .5*range, b -= .5*range;
        r += .5*range, t += .5*range;
        if (miterLimit > 0)
            shape.boundMiters(l, b, r, t, .5*range, miterLimit, 1);
        double w = scale*(r-l);
        double h = scale*(t-b);
        box.rect.w = (int) ceil(w)+1;
        box.rect.h = (int) ceil(h)+1;
        box.translate.x = -l+.5*(box.rect.w-w)/scale;
        box.translate.y = -b+.5*(box.rect.h-h)/scale;
    } else {
        box.rect.w = 0, box.rect.h = 0;
        box.translate = msdfgen::Vector2();
    }
}

void GlyphGeometry::placeBox(int x, int y) {
    box.rect.x = x, box.rect.y = y;
}

int GlyphGeometry::getIndex() const {
    return index;
}

msdfgen::GlyphIndex GlyphGeometry::getGlyphIndex() const {
    return msdfgen::GlyphIndex(index);
}

unicode_t GlyphGeometry::getCodepoint() const {
    return codepoint;
}

int GlyphGeometry::getIdentifier(GlyphIdentifierType type) const {
    switch (type) {
        case GlyphIdentifierType::GLYPH_INDEX:
            return index;
        case GlyphIdentifierType::UNICODE_CODEPOINT:
            return (int) codepoint;
    }
    return 0;
}

const msdfgen::Shape & GlyphGeometry::getShape() const {
    return shape;
}

double GlyphGeometry::getAdvance() const {
    return advance;
}

void GlyphGeometry::getBoxRect(int &x, int &y, int &w, int &h) const {
    x = box.rect.x, y = box.rect.y;
    w = box.rect.w, h = box.rect.h;
}

void GlyphGeometry::getBoxSize(int &w, int &h) const {
    w = box.rect.w, h = box.rect.h;
}

double GlyphGeometry::getBoxRange() const {
    return box.range;
}

msdfgen::Projection GlyphGeometry::getBoxProjection() const {
    return msdfgen::Projection(msdfgen::Vector2(box.scale), box.translate);
}

double GlyphGeometry::getBoxScale() const {
    return box.scale;
}

msdfgen::Vector2 GlyphGeometry::getBoxTranslate() const {
    return box.translate;
}

void GlyphGeometry::getQuadPlaneBounds(double &l, double &b, double &r, double &t) const {
    if (box.rect.w > 0 && box.rect.h > 0) {
        double invBoxScale = 1/box.scale;
        l = geometryScale*(-box.translate.x+.5*invBoxScale);
        b = geometryScale*(-box.translate.y+.5*invBoxScale);
        r = geometryScale*(-box.translate.x+(box.rect.w-.5)*invBoxScale);
        t = geometryScale*(-box.translate.y+(box.rect.h-.5)*invBoxScale);
    } else
        l = 0, b = 0, r = 0, t = 0;
}

void GlyphGeometry::getQuadAtlasBounds(double &l, double &b, double &r, double &t) const {
    if (box.rect.w > 0 && box.rect.h > 0) {
        l = box.rect.x+.5;
        b = box.rect.y+.5;
        r = box.rect.x+box.rect.w-.5;
        t = box.rect.y+box.rect.h-.5;
    } else
        l = 0, b = 0, r = 0, t = 0;
}

bool GlyphGeometry::isWhitespace() const {
    return shape.contours.empty();
}

GlyphGeometry::operator GlyphBox() const {
    GlyphBox box;
    box.index = index;
    box.advance = advance;
    getQuadPlaneBounds(box.bounds.l, box.bounds.b, box.bounds.r, box.bounds.t);
    box.rect.x = this->box.rect.x, box.rect.y = this->box.rect.y, box.rect.w = this->box.rect.w, box.rect.h = this->box.rect.h;
    return box;
}

}
