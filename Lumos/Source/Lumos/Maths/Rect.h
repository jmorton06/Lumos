#pragma once
#include "MathsUtilities.h"
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>

namespace Lumos
{
    namespace Maths
    {
        class Rect
        {
        public:
            Rect();
            Rect(const glm::vec2& position, const glm::vec2& size);
            Rect(const glm::vec4& rect);
            Rect(float x, float y, float width, float height);

            void SetPosition(const glm::vec2& position);
            void SetSize(const glm::vec2& size);
            void Set(const glm::vec2& position, const glm::vec2& size);
            void Set(float x, float y, float width, float height);

            const glm::vec2& GetPosition() const;
            const glm::vec2& GetSize() const;
            const glm::vec4& Get() const;

            bool IsInside(const glm::vec2& point) const;
            bool IsInside(float x, float y) const;
            bool IsInside(const Rect& rect) const;

            bool Intersects(const Rect& rect) const;

            void Rotate(float angle);

            void Transform(const glm::mat4& transform);

            void SetCenter(const glm::vec2& center);
            void SetCenter(float x, float y);

            const glm::vec2& GetCenter() const;

        private:
            glm::vec2 m_Position;
            glm::vec2 m_Size;
            glm::vec4 m_Rect;
            glm::vec2 m_Center;
            float m_Radius;
            float m_Angle;
        };
    }
}
