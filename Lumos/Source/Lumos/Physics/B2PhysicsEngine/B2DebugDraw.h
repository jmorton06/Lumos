#pragma once
#include <box2d/box2d.h>
#include <box2d/types.h>

namespace Lumos
{

    //    class B2DebugDraw : public b2Draw
    //    {
    //    public:
    //        B2DebugDraw()  = default;
    //        ~B2DebugDraw() = default;
    //
    namespace B2DebugDraw
    {
        void DrawPolygon(const b2Vec2* vertices, int vertexCount, b2HexColor colour, void* context);
        void DrawSolidPolygon(b2Transform xf, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor colour, void* context);
        void DrawCircle(b2Vec2 center, float radius, b2HexColor colour, void* context);
        void DrawSolidCircle(b2Transform xf, float radius, b2HexColor colour, void* context);
        void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2HexColor colour, void* context);
        void DrawTransform(b2Transform xf, void* context);
        void DrawPoint(b2Vec2 p, float size, b2HexColor colour, void* context);
        void DrawString(b2Vec2 p, const char* s, void* context);
    }
    //
    //        void DrawString(int x, int y, const char* string, ...);
    //
    //        void DrawString(b2Vec2 p, const char* string, ...);
    //
    //        void DrawAABB(b2AABB* aabb, const b2Color& colour);
    //    };
}
