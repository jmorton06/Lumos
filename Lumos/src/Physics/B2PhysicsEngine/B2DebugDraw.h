#include "lmpch.h"

#include <Box2D/Box2D.h>

namespace Lumos {

    class B2DebugDraw : public b2Draw
    {
    public:
        B2DebugDraw() = default;
        ~B2DebugDraw() = default;

        void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

        void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

        void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;

        void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;

        void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;

        void DrawTransform(const b2Transform& xf) override;

        void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

        void DrawString(int x, int y, const char* string, ...); 

        void DrawString(const b2Vec2& p, const char* string, ...);

        void DrawAABB(b2AABB* aabb, const b2Color& color);
    };
}
