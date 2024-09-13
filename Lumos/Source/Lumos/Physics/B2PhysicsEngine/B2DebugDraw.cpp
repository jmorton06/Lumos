#include "Precompiled.h"
#include "B2DebugDraw.h"

#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    static inline Vec4 MakeRGBA8(b2HexColor c, float alpha)
    {
        return { static_cast<float>(uint8_t((c >> 16) & 0xFF)), static_cast<float>(uint8_t((c >> 8) & 0xFF)), static_cast<float>(uint8_t(c & 0xFF)), static_cast<float>(uint8_t(0xFF * alpha)) };
    }

    void B2DebugDraw::DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor hexColour, void* context)
    {
        Vec4 colour = MakeRGBA8(hexColour, 0.8f);

        b2Vec2 p1 = vertices[vertexCount - 1];
        for(int i = 0; i < vertexCount; ++i)
        {
            b2Vec2 p2 = vertices[i];
            DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });
            p1 = p2;
        }
    }

    //
    void B2DebugDraw::DrawSolidPolygon(b2Transform xf, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor hexColour, void* context)
    {
        Vec4 colour = MakeRGBA8(hexColour, 0.8f);
        Vec4 fillColour(0.5f * colour.x, 0.5f * colour.y, 0.5f * colour.z, 0.5f);

        for(int i = 1; i < vertexCount - 1; ++i)
        {
            b2Vec2 v0 = b2TransformPoint(xf, vertices[0]);
            b2Vec2 v1 = b2TransformPoint(xf, vertices[1]);
            b2Vec2 v2 = b2TransformPoint(xf, vertices[2]);

            DebugRenderer::DrawTriangle({ v0.x, v0.y, 0.0f }, { v2.x, v2.y, 0.0f }, { v1.x, v1.y, 0.0f }, false, { fillColour.x, fillColour.y, fillColour.z, fillColour.w });
        }

        b2Vec2 p1 = b2TransformPoint(xf, vertices[vertexCount - 1]);
        for(int i = 0; i < vertexCount; ++i)
        {
            b2Vec2 p2 = b2TransformPoint(xf, vertices[i]);
            DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });
            p1 = p2;
        }
    }

    //
    void B2DebugDraw::DrawCircle(b2Vec2 center, float radius, b2HexColor hexColour, void* context)
    {
        Vec4 colour             = MakeRGBA8(hexColour, 0.8f);
        const float k_segments  = 16.0f;
        const float k_increment = 2.0f * b2_pi / k_segments;
        float sinInc            = sinf(k_increment);
        float cosInc            = cosf(k_increment);
        b2Vec2 r1               = { 1.0f, 0.0f };
        b2Vec2 v1               = center + radius * r1;
        for(int i = 0; i < k_segments; ++i)
        {
            // Perform rotation to avoid additional trigonometry.
            b2Vec2 r2;
            r2.x      = cosInc * r1.x - sinInc * r1.y;
            r2.y      = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = center + radius * r2;
            DebugRenderer::DrawHairLine({ v1.x, v1.y, 0.0f }, { v2.x, v2.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });
            r1 = r2;
            v1 = v2;
        }
    }

    //
    void B2DebugDraw::DrawSolidCircle(b2Transform xf, float radius, b2HexColor hexColour, void* context)
    {
        Vec4 colour             = MakeRGBA8(hexColour, 0.8f);
        const float k_segments  = 16.0f;
        const float k_increment = 2.0f * b2_pi / k_segments;
        float sinInc            = sinf(k_increment);
        float cosInc            = cosf(k_increment);
        b2Vec2 v0               = b2TransformPoint(xf, { 0.0f, 0.0f });
        b2Vec2 r1               = { cosInc, sinInc };
        b2Vec2 v1               = v0 + radius * r1;
        Vec4 fillColour(0.5f * colour.x, 0.5f * colour.y, 0.5f * colour.z, 0.5f);
        for(int i = 0; i < k_segments; ++i)
        {
            // Perform rotation to avoid additional trigonometry.
            b2Vec2 r2;
            r2.x      = cosInc * r1.x - sinInc * r1.y;
            r2.y      = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = v0 + radius * r2;
            DebugRenderer::DrawTriangle({ v0.x, v0.y, 0.0f }, { v2.x, v2.y, 0.0f }, { v1.x, v1.y, 0.0f }, false, { fillColour.x, fillColour.y, fillColour.z, fillColour.w });
            r1 = r2;
            v1 = v2;
        }

        r1 = { 1.0f, 0.0f };
        v1 = v0 + radius * r1;
        for(int i = 0; i < k_segments; ++i)
        {
            b2Vec2 r2;
            r2.x      = cosInc * r1.x - sinInc * r1.y;
            r2.y      = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = v0 + radius * r2;
            DebugRenderer::DrawHairLine({ v1.x, v1.y, 0.0f }, { v2.x, v2.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });

            r1 = r2;
            v1 = v2;
        }

        // Draw a line fixed in the circle to animate rotation.
        // b2Vec2 p = v0 + radius * axis;
        // DebugRenderer::DrawHairLine({ p.x, p.y, 0.0f }, { v0.x, v0.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });
    }

    //
    void B2DebugDraw::DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor hexColour, void* context)
    {
        Vec4 colour = MakeRGBA8(hexColour, 0.8f);
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, false, { colour.x, colour.y, colour.z, colour.w });
    }

    //
    void B2DebugDraw::DrawTransform(b2Transform xf, void* context)
    {
        const float k_axisScale = 0.4f;
        Vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
        Vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
        b2Vec2 p1 = xf.p, p2;

        // p2 = p1 + k_axisScale * xf.q.GetXAxis();
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, false, { red.x, red.y, red.z, red.w });

        // p2 = p1 + k_axisScale * xf.q.GetYAxis();
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, false, { green.x, green.y, green.z, green.w });
    }

    //
    void B2DebugDraw::DrawPoint(b2Vec2 p, float size, b2HexColor hexColour, void* context)
    {
        Vec4 colour = MakeRGBA8(hexColour, 0.8f);
        DebugRenderer::DrawPoint({ p.x, p.y, 0.0f }, size * 0.1f, false, { colour.x, colour.y, colour.z, colour.w });
    }

    void B2DebugDraw::DrawString(b2Vec2 p, const char* s, void* context)
    {
        DebugRenderer::DrawTextCs({ p.x, p.y, 0.0f, 0.0f }, 12.0f, s);
    }
}
