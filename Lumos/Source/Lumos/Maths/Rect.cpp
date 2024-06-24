#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Rect.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Lumos
{
    namespace Maths
    {
        Rect::Rect()
        {
            m_Position = glm::vec2(0.0f, 0.0f);
            m_Size     = glm::vec2(0.0f, 0.0f);
            m_Rect     = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            m_Center   = glm::vec2(0.0f, 0.0f);
            m_Radius   = 0.0f;
            m_Angle    = 0.0f;
        }

        Rect::Rect(const glm::vec2& position, const glm::vec2& size)
        {
            m_Position = position;
            m_Size     = size;
            m_Rect     = glm::vec4(position.x, position.y, size.x, size.y);
            m_Center   = position + size * 0.5f;
            m_Radius   = size.x * 0.5f;
            m_Angle    = 0.0f;
        }

        Rect::Rect(const glm::vec4& rect)
        {
            m_Position = glm::vec2(rect.x, rect.y);
            m_Size     = glm::vec2(rect.z, rect.w);
            m_Rect     = glm::vec4(rect.x, rect.y, rect.z, rect.w);
            m_Center   = m_Position + m_Size * 0.5f;
            m_Radius   = m_Size.x * 0.5f;
            m_Angle    = 0.0f;
        }

        Rect::Rect(float x, float y, float width, float height)
        {
            m_Position = glm::vec2(x, y);
            m_Size     = glm::vec2(width, height);
            m_Rect     = glm::vec4(x, y, width, height);
            m_Center   = m_Position + m_Size * 0.5f;
            m_Radius   = m_Size.x * 0.5f;
            m_Angle    = 0.0f;
        }

        const glm::vec2& Rect::GetPosition() const
        {
            return m_Position;
        }
        const glm::vec2& Rect::GetSize() const
        {
            return m_Size;
        }

        const glm::vec4& Rect::Get() const
        {
            return m_Rect;
        }

        void Rect::SetPosition(const glm::vec2& position)
        {
            m_Position = position;
            m_Rect.x   = position.x;
            m_Rect.y   = position.y;
            m_Center   = m_Position + m_Size * 0.5f;
        }

        void Rect::SetSize(const glm::vec2& size)
        {
            m_Size   = size;
            m_Rect.z = size.x;
            m_Rect.w = size.y;
            m_Center = m_Position + m_Size * 0.5f;
            m_Radius = m_Size.x * 0.5f;
        }

        void Rect::Set(const glm::vec2& position, const glm::vec2& size)
        {
            m_Position = position;
            m_Size     = size;
            m_Rect.x   = position.x;
            m_Rect.y   = position.y;
            m_Rect.z   = size.x;
            m_Rect.w   = size.y;
            m_Center   = m_Position + m_Size * 0.5f;
            m_Radius   = m_Size.x * 0.5f;
        }

        void Rect::Set(float x, float y, float width, float height)
        {
            m_Position = glm::vec2(x, y);
            m_Size     = glm::vec2(width, height);
            m_Rect.x   = x;
            m_Rect.y   = y;
            m_Rect.z   = width;
            m_Rect.w   = height;
            m_Center   = m_Position + m_Size * 0.5f;
            m_Radius   = m_Size.x * 0.5f;
        }

        void Rect::Transform(const glm::mat4& transform)
        {
            m_Rect     = transform * m_Rect;
            m_Position = glm::vec2(m_Rect.x, m_Rect.y);
            m_Size     = glm::vec2(m_Rect.z, m_Rect.w);
            m_Center   = m_Position + m_Size * 0.5f;
            m_Radius   = m_Size.x * 0.5f;
        }

        void Rect::SetCenter(const glm::vec2& center)
        {
            m_Position = center - m_Size * 0.5f;
            m_Rect.x   = center.x - m_Size.x * 0.5f;
            m_Rect.y   = center.y - m_Size.y * 0.5f;
            m_Center   = center;
        }

        void Rect::SetCenter(float x, float y)
        {
            m_Position.x = x - m_Size.x * 0.5f;
            m_Position.y = y - m_Size.y * 0.5f;
            m_Rect.x     = x - m_Size.x * 0.5f;
            m_Rect.y     = y - m_Size.y * 0.5f;
            m_Center.x   = x;
            m_Center.y   = y;
        }

        bool Rect::IsInside(const glm::vec2& point) const
        {
            if(point.x < m_Position.x || point.x > m_Position.x + m_Size.x)
            {
                return false;
            }

            if(point.y < m_Position.y || point.y > m_Position.y + m_Size.y)
            {
                return false;
            }

            return true;
        }

        bool Rect::IsInside(float x, float y) const
        {
            if(x < m_Position.x || x > m_Position.x + m_Size.x)
            {
                return false;
            }

            if(y < m_Position.y || y > m_Position.y + m_Size.y)
            {
                return false;
            }

            return true;
        }

        bool Rect::IsInside(const Rect& rect) const
        {
            if(rect.m_Position.x > m_Position.x + m_Size.x || rect.m_Position.x + rect.m_Size.x < m_Position.x)
            {
                return false;
            }

            if(rect.m_Position.y > m_Position.y + m_Size.y || rect.m_Position.y + rect.m_Size.y < m_Position.y)
            {
                return false;
            }

            return true;
        }

        bool Rect::Intersects(const Rect& rect) const
        {
            if(rect.m_Position.x > m_Position.x + m_Size.x || rect.m_Position.x + rect.m_Size.x < m_Position.x)
            {
                return false;
            }

            if(rect.m_Position.y > m_Position.y + m_Size.y || rect.m_Position.y + rect.m_Size.y < m_Position.y)
            {
                return false;
            }

            return true;
        }
    }
}
