#include "Precompiled.h"
#include "B2DebugDraw.h"

#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    void B2DebugDraw::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& colour)
    {
        b2Vec2 p1 = vertices[vertexCount - 1];
        for(int32 i = 0; i < vertexCount; ++i)
        {
            b2Vec2 p2 = vertices[i];
            DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });
            p1 = p2;
        }
    }

    //
    void B2DebugDraw::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& colour)
    {
        b2Color fillColour(0.5f * colour.r, 0.5f * colour.g, 0.5f * colour.b, 0.5f);

        for(int32 i = 1; i < vertexCount - 1; ++i)
        {
            DebugRenderer::DrawTriangle({ vertices[0].x, vertices[0].y, 0.0f }, { vertices[2].x, vertices[2].y, 0.0f }, { vertices[1].x, vertices[1].y, 0.0f }, { fillColour.r, fillColour.g, fillColour.b, fillColour.a });
        }

        b2Vec2 p1 = vertices[vertexCount - 1];
        for(int32 i = 0; i < vertexCount; ++i)
        {
            b2Vec2 p2 = vertices[i];
            DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });
            p1 = p2;
        }
    }

    //
    void B2DebugDraw::DrawCircle(const b2Vec2& center, float radius, const b2Color& colour)
    {
        const float k_segments = 16.0f;
        const float k_increment = 2.0f * b2_pi / k_segments;
        float sinInc = sinf(k_increment);
        float cosInc = cosf(k_increment);
        b2Vec2 r1(1.0f, 0.0f);
        b2Vec2 v1 = center + radius * r1;
        for(int32 i = 0; i < k_segments; ++i)
        {
            // Perform rotation to avoid additional trigonometry.
            b2Vec2 r2;
            r2.x = cosInc * r1.x - sinInc * r1.y;
            r2.y = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = center + radius * r2;
            DebugRenderer::DrawHairLine({ v1.x, v1.y, 0.0f }, { v2.x, v2.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });
            r1 = r2;
            v1 = v2;
        }
    }

    //
    void B2DebugDraw::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& colour)
    {
        const float k_segments = 16.0f;
        const float k_increment = 2.0f * b2_pi / k_segments;
        float sinInc = sinf(k_increment);
        float cosInc = cosf(k_increment);
        b2Vec2 v0 = center;
        b2Vec2 r1(cosInc, sinInc);
        b2Vec2 v1 = center + radius * r1;
        b2Color fillColour(0.5f * colour.r, 0.5f * colour.g, 0.5f * colour.b, 0.5f);
        for(int32 i = 0; i < k_segments; ++i)
        {
            // Perform rotation to avoid additional trigonometry.
            b2Vec2 r2;
            r2.x = cosInc * r1.x - sinInc * r1.y;
            r2.y = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = center + radius * r2;
            DebugRenderer::DrawTriangle({ v0.x, v0.y, 0.0f }, { v2.x, v2.y, 0.0f }, { v1.x, v1.y, 0.0f }, { fillColour.r, fillColour.g, fillColour.b, fillColour.a });
            r1 = r2;
            v1 = v2;
        }

        r1.Set(1.0f, 0.0f);
        v1 = center + radius * r1;
        for(int32 i = 0; i < k_segments; ++i)
        {
            b2Vec2 r2;
            r2.x = cosInc * r1.x - sinInc * r1.y;
            r2.y = sinInc * r1.x + cosInc * r1.y;
            b2Vec2 v2 = center + radius * r2;
            DebugRenderer::DrawHairLine({ v1.x, v1.y, 0.0f }, { v2.x, v2.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });

            r1 = r2;
            v1 = v2;
        }

        // Draw a line fixed in the circle to animate rotation.
        b2Vec2 p = center + radius * axis;
        DebugRenderer::DrawHairLine({ p.x, p.y, 0.0f }, { center.x, center.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });
    }

    //
    void B2DebugDraw::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& colour)
    {
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, { colour.r, colour.g, colour.b, colour.a });
    }

    //
    void B2DebugDraw::DrawTransform(const b2Transform& xf)
    {
        const float k_axisScale = 0.4f;
        b2Color red(1.0f, 0.0f, 0.0f);
        b2Color green(0.0f, 1.0f, 0.0f);
        b2Vec2 p1 = xf.p, p2;

        p2 = p1 + k_axisScale * xf.q.GetXAxis();
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, { red.r, red.g, red.b, red.a });

        p2 = p1 + k_axisScale * xf.q.GetYAxis();
        DebugRenderer::DrawHairLine({ p1.x, p1.y, 0.0f }, { p2.x, p2.y, 0.0f }, { green.r, green.g, green.b, green.a });
    }

    //
    void B2DebugDraw::DrawPoint(const b2Vec2& p, float size, const b2Color& colour)
    {
        DebugRenderer::DrawPoint({ p.x, p.y }, size, { colour.r, colour.g, colour.b, colour.a });
    }

}
