#pragma once
#include "Maths/MathsFwd.h"
#include "Maths/Vector2.h"
#include "Maths/Vector4.h"

namespace Lumos
{
    namespace Maths
    {
        class Rect
        {
        public:
            Rect();
            Rect(const Vec2& position, const Vec2& size);
            Rect(const Vec4& rect);
            Rect(float x, float y, float width, float height);

            void SetPosition(const Vec2& position);
            void SetSize(const Vec2& size);
            void Set(const Vec2& position, const Vec2& size);
            void Set(float x, float y, float width, float height);

            const Vec2& GetPosition() const;
            const Vec2& GetSize() const;
            const Vec4& Get() const;

            bool IsInside(const Vec2& point) const;
            bool IsInside(float x, float y) const;
            bool IsInside(const Rect& rect) const;

            bool Intersects(const Rect& rect) const;

            void Rotate(float angle);

            void Transform(const Mat4& transform);

            void SetCenter(const Vec2& center);
            void SetCenter(float x, float y);

            const Vec2& GetCenter() const;

        private:
            Vec2 m_Position;
            Vec2 m_Size;
            Vec4 m_Rect;
            Vec2 m_Center;
            float m_Radius;
            float m_Angle;
        };
    }
}
