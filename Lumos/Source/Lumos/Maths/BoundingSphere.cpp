#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Frustum.h"
#include "Maths/MathsUtilities.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtx/norm.hpp>

namespace Lumos
{
    namespace Maths
    {
        BoundingSphere::BoundingSphere()
        {
            m_Center = glm::vec3(0.0f);
            m_Radius = 0.0f;
        }

        BoundingSphere::BoundingSphere(const glm::vec3& center, float radius)
        {
            m_Center = center;
            m_Radius = radius;
        }

        BoundingSphere::BoundingSphere(const BoundingSphere& copy)
        {
            m_Center = copy.m_Center;
            m_Radius = copy.m_Radius;
        }

        BoundingSphere::BoundingSphere(const glm::vec3* points, unsigned int count)
        {
            if(count == 0)
            {
                m_Center = glm::vec3(0.0f);
                m_Radius = 0.0f;
                return;
            }

            m_Center = glm::vec3(0.0f);
            m_Radius = 0.0f;

            for(unsigned int i = 0; i < count; i++)
            {
                m_Center += points[i];
            }

            m_Center /= (float)count;

            float maxDist = 0.0f;
            for(unsigned int i = 0; i < count; i++)
            {
                float dist = glm::distance(points[i], m_Center);
                if(dist > maxDist)
                {
                    maxDist = dist;
                }
            }

            m_Radius = maxDist;
        }

        BoundingSphere::BoundingSphere(const glm::vec3* points, unsigned int count, const glm::vec3& center)
        {
            if(count == 0)
            {
                m_Center = glm::vec3(0.0f);
                m_Radius = 0.0f;
                return;
            }

            m_Center = center;
            m_Radius = 0.0f;

            for(unsigned int i = 0; i < count; i++)
            {
                m_Center += points[i];
            }

            m_Center /= (float)count;

            float maxDistSq = 0.0f;
            for(unsigned int i = 0; i < count; i++)
            {
                float dist = glm::length(points[i] - m_Center);
                if(dist > maxDistSq)
                {
                    maxDistSq = dist;
                }
            }

            m_Radius = maxDistSq;
        }

        BoundingSphere::BoundingSphere(const glm::vec3* points, unsigned int count, const glm::vec3& center, float radius)
        {
            if(count == 0)
            {
                m_Center = glm::vec3(0.0f);
                m_Radius = 0.0f;
                return;
            }

            m_Center = center;
            m_Radius = radius;

            for(unsigned int i = 0; i < count; i++)
            {
                m_Center += points[i];
            }

            m_Center /= (float)count;

            float maxDistSq = 0.0f;
            for(unsigned int i = 0; i < count; i++)
            {
                float dist = glm::length(points[i] - m_Center);
                if(dist > maxDistSq)
                {
                    maxDistSq = dist;
                }
            }

            m_Radius = maxDistSq;
        }

        BoundingSphere& BoundingSphere::operator=(const BoundingSphere& rhs)
        {
            if(this == &rhs)
                return *this;

            m_Center = rhs.m_Center;
            m_Radius = rhs.m_Radius;

            return *this;
        }

        bool BoundingSphere::IsInside(const glm::vec3& point) const
        {
            return glm::length2(point - m_Center) <= m_Radius * m_Radius;
        }

        bool BoundingSphere::IsInside(const BoundingSphere& sphere) const
        {
            return glm::length2(sphere.m_Center - m_Center) <= (m_Radius + sphere.m_Radius) * (m_Radius + sphere.m_Radius);
        }

        bool BoundingSphere::IsInside(const BoundingBox& box) const
        {
            return box.IsInside(*this);
        }

        bool BoundingSphere::IsInside(const Frustum& frustum) const
        {
            return frustum.IsInside(*this);
        }

        const glm::vec3& BoundingSphere::GetCenter() const
        {
            return m_Center;
        }

        float BoundingSphere::GetRadius() const
        {
            return m_Radius;
        }

        void BoundingSphere::SetCenter(const glm::vec3& center)
        {
            m_Center = center;
        }

        void BoundingSphere::SetRadius(float radius)
        {
            m_Radius = radius;
        }

        bool BoundingSphere::Contains(const glm::vec3& point) const
        {
            return glm::length2(point - m_Center) <= m_Radius * m_Radius;
        }

        bool BoundingSphere::Contains(const BoundingSphere& other) const
        {
            return glm::distance2(other.m_Center, m_Center) <= (m_Radius + other.m_Radius) * (m_Radius + other.m_Radius);
        }
        bool BoundingSphere::Intersects(const BoundingSphere& other) const
        {
            return glm::distance2(other.m_Center, m_Center) <= (m_Radius + other.m_Radius) * (m_Radius + other.m_Radius);
        }
        bool BoundingSphere::Intersects(const glm::vec3& point) const
        {
            return glm::distance2(point, m_Center) <= m_Radius * m_Radius;
        }
        bool BoundingSphere::Intersects(const glm::vec3& point, float radius) const
        {
            return glm::distance2(point, m_Center) <= (m_Radius + radius) * (m_Radius + radius);
        }

        void BoundingSphere::Merge(const BoundingSphere& other)
        {
            float distance = glm::distance(other.m_Center, m_Center);

            if(distance > m_Radius + other.m_Radius)
                return;

            if(distance <= m_Radius - other.m_Radius)
            {
                m_Center = other.m_Center;
                m_Radius = other.m_Radius;
                return;
            }

            if(distance <= other.m_Radius - m_Radius)
                return;

            float half  = (distance + m_Radius + other.m_Radius) * 0.5f;
            float scale = half / distance;
            m_Center    = (m_Center + other.m_Center) * scale;
            m_Radius    = half;
        }

        void BoundingSphere::Merge(const glm::vec3& point)
        {
            float distance = glm::distance(point, m_Center);

            if(distance > m_Radius)
                return;

            if(distance <= 0.0f)
            {
                m_Center = point;
                m_Radius = 0.0f;
                return;
            }

            float half  = (distance + m_Radius) * 0.5f;
            float scale = half / distance;
            m_Center    = (m_Center + point) * scale;
            m_Radius    = half;
        }

        void BoundingSphere::Merge(const glm::vec3* points, unsigned int count)
        {
            if(count == 0)
                return;

            float radius     = 0.0f;
            glm::vec3 center = points[0];

            for(unsigned int i = 1; i < count; i++)
            {
                float distance = glm::distance(points[i], center);

                if(distance > radius)
                    radius = distance;

                center += points[i];
            }

            center /= (float)count;

            float distance = glm::distance(center, m_Center);

            if(distance > m_Radius)
                return;

            if(distance <= 0.0f)
            {
                m_Center = center;
                m_Radius = 0.0f;
                return;
            }

            float half  = (distance + m_Radius + radius) * 0.5f;
            float scale = half / distance;
            m_Center    = (m_Center + center) * scale;
            m_Radius    = half;
        }

        void BoundingSphere::Transform(const glm::mat4& transform)
        {
            glm::vec3 center = transform * glm::vec4(m_Center, 1.0f);

            m_Center = center;
        }
    }
}
