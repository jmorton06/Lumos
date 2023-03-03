#include "Precompiled.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"

#include <glm/mat4x4.hpp>

namespace Lumos
{
    namespace Maths
    {
        BoundingBox::BoundingBox()
        {
            m_Min = glm::vec3(FLT_MAX);
            m_Max = glm::vec3(-FLT_MAX);
        }

        BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max)
        {
            m_Min = min;
            m_Max = max;
        }

        BoundingBox::BoundingBox(const Rect& rect, const glm::vec3& center)
        {
            m_Min = center + glm::vec3(rect.GetPosition(), 0.0f);
            m_Max = center + glm::vec3(rect.GetPosition().x + rect.GetSize().x, rect.GetPosition().y + rect.GetSize().y, 0.0f);
        }

        BoundingBox::BoundingBox(const BoundingBox& other)
        {
            m_Min = other.m_Min;
            m_Max = other.m_Max;
        }

        BoundingBox::BoundingBox(const glm::vec3* points, uint32_t numPoints)
        {
            SetFromPoints(points, numPoints);
        }

        BoundingBox::BoundingBox(BoundingBox&& other)
        {
            m_Min = other.m_Min;
            m_Max = other.m_Max;
        }

        BoundingBox::~BoundingBox()
        {
        }

        BoundingBox& BoundingBox::operator=(const BoundingBox& other)
        {
            m_Min = other.m_Min;
            m_Max = other.m_Max;

            return *this;
        }

        BoundingBox& BoundingBox::operator=(BoundingBox&& other)
        {
            m_Min = other.m_Min;
            m_Max = other.m_Max;

            return *this;
        }

        void BoundingBox::Set(const glm::vec3& min, const glm::vec3& max)
        {
            m_Min = min;
            m_Max = max;
        }

        void BoundingBox::Set(const glm::vec3* points, uint32_t numPoints)
        {
            m_Min = glm::vec3(FLT_MAX);
            m_Max = glm::vec3(-FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                m_Min = glm::min(m_Min, points[i]);
                m_Max = glm::max(m_Max, points[i]);
            }
        }

        void BoundingBox::SetFromPoints(const glm::vec3* points, uint32_t numPoints)
        {
            m_Min = glm::vec3(FLT_MAX);
            m_Max = glm::vec3(-FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                m_Min = glm::min(m_Min, points[i]);
                m_Max = glm::max(m_Max, points[i]);
            }
        }

        void BoundingBox::SetFromPoints(const glm::vec3* points, uint32_t numPoints, const glm::mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
            glm::vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                glm::vec3 transformed = transform * glm::vec4(points[i], 1.0f);

                min.x = glm::min(min.x, transformed.x);
                min.y = glm::min(min.y, transformed.y);
                min.z = glm::min(min.z, transformed.z);

                max.x = glm::max(max.x, transformed.x);
                max.y = glm::max(max.y, transformed.y);
                max.z = glm::max(max.z, transformed.z);
            }

            m_Min = min;
            m_Max = max;
        }

        void BoundingBox::SetFromTransformedAABB(const BoundingBox& aabb, const glm::mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 min = aabb.m_Min;
            glm::vec3 max = aabb.m_Max;

            glm::vec3 minTransformed = transform * glm::vec4(min, 1.0f);
            glm::vec3 maxTransformed = transform * glm::vec4(max, 1.0f);

            m_Min = glm::min(minTransformed, maxTransformed);
            m_Max = glm::max(minTransformed, maxTransformed);
        }

        void BoundingBox::Translate(const glm::vec3& translation)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Min += translation;
            m_Max += translation;
        }

        void BoundingBox::Translate(float x, float y, float z)
        {
            LUMOS_PROFILE_FUNCTION();
            Translate(glm::vec3(x, y, z));
        }

        void BoundingBox::Scale(const glm::vec3& scale)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Min *= scale;
            m_Max *= scale;
        }

        void BoundingBox::Scale(float x, float y, float z)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Min.x *= x;
            m_Min.y *= y;
            m_Min.z *= z;

            m_Max.x *= x;
            m_Max.y *= y;
            m_Max.z *= z;
        }

        void BoundingBox::Rotate(const glm::mat3& rotation)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 center  = Center();
            glm::vec3 extents = GetExtents();

            glm::vec3 rotatedExtents = glm::vec3(rotation * glm::vec4(extents, 1.0f));

            m_Min = center - rotatedExtents;
            m_Max = center + rotatedExtents;
        }

        void BoundingBox::Transform(const glm::mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 newCenter = transform * glm::vec4(Center(), 1.0f);
            glm::vec3 oldEdge   = Size() * 0.5f;
            glm::vec3 newEdge   = glm::vec3(
                glm::abs(transform[0][0]) * oldEdge.x + glm::abs(transform[1][0]) * oldEdge.y + glm::abs(transform[2][0]) * oldEdge.z,
                glm::abs(transform[0][1]) * oldEdge.x + glm::abs(transform[1][1]) * oldEdge.y + glm::abs(transform[2][1]) * oldEdge.z,
                glm::abs(transform[0][2]) * oldEdge.x + glm::abs(transform[1][2]) * oldEdge.y + glm::abs(transform[2][2]) * oldEdge.z);

            m_Min = newCenter - newEdge;
            m_Max = newCenter + newEdge;
        }

        BoundingBox BoundingBox::Transformed(const glm::mat4& transform) const
        {
            LUMOS_PROFILE_FUNCTION();
            BoundingBox box(*this);
            box.Transform(transform);
            return box;
        }

        void BoundingBox::Merge(const BoundingBox& other)
        {
            LUMOS_PROFILE_FUNCTION();
            if(other.m_Min.x < m_Min.x)
                m_Min.x = other.m_Min.x;
            if(other.m_Min.y < m_Min.y)
                m_Min.y = other.m_Min.y;
            if(other.m_Min.z < m_Min.z)
                m_Min.z = other.m_Min.z;
            if(other.m_Max.x > m_Max.x)
                m_Max.x = other.m_Max.x;
            if(other.m_Max.y > m_Max.y)
                m_Max.y = other.m_Max.y;
            if(other.m_Max.z > m_Max.z)
                m_Max.z = other.m_Max.z;
        }

        void BoundingBox::Merge(const glm::vec3& point)
        {
            LUMOS_PROFILE_FUNCTION();
            if(point.x < m_Min.x)
                m_Min.x = point.x;
            if(point.y < m_Min.y)
                m_Min.y = point.y;
            if(point.z < m_Min.z)
                m_Min.z = point.z;
            if(point.x > m_Max.x)
                m_Max.x = point.x;
            if(point.y > m_Max.y)
                m_Max.y = point.y;
            if(point.z > m_Max.z)
                m_Max.z = point.z;
        }

        void BoundingBox::Merge(const BoundingBox& other, const glm::mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            Merge(other.Transformed(transform));
        }
        void BoundingBox::Merge(const glm::vec3& point, const glm::mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 transformed = transform * glm::vec4(point, 1.0f);
            Merge(transformed);
        }

        void BoundingBox::Merge(const BoundingBox& other, const glm::mat3& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            Merge(other.Transformed(glm::mat4(transform)));
        }
        void BoundingBox::Merge(const glm::vec3& point, const glm::mat3& transform)
        {
            LUMOS_PROFILE_FUNCTION();
            glm::vec3 transformed = transform * glm::vec4(point, 1.0f);
            Merge(transformed);
        }

        Intersection BoundingBox::IsInside(const glm::vec3& point) const
        {
            LUMOS_PROFILE_FUNCTION();
            if(point.x < m_Min.x || point.x > m_Max.x || point.y < m_Min.y || point.y > m_Max.y || point.z < m_Min.z || point.z > m_Max.z)
                return OUTSIDE;
            else
                return INSIDE;
        }

        Intersection BoundingBox::IsInside(const BoundingBox& box) const
        {
            LUMOS_PROFILE_FUNCTION();
            if(box.m_Max.x < m_Min.x || box.m_Min.x > m_Max.x || box.m_Max.y < m_Min.y || box.m_Min.y > m_Max.y || box.m_Max.z < m_Min.z || box.m_Min.z > m_Max.z)
                return OUTSIDE;
            else if(box.m_Min.x < m_Min.x || box.m_Max.x > m_Max.x || box.m_Min.y < m_Min.y || box.m_Max.y > m_Max.y || box.m_Min.z < m_Min.z || box.m_Max.z > m_Max.z)
                return INTERSECTS;
            else
                return INSIDE;
        }

        Intersection BoundingBox::IsInside(const BoundingSphere& sphere) const
        {
            LUMOS_PROFILE_FUNCTION();
            float distSquared = 0;
            float temp;
            const glm::vec3& center = sphere.GetCenter();

            if(center.x < m_Min.x)
            {
                temp = center.x - m_Min.x;
                distSquared += temp * temp;
            }
            else if(center.x > m_Max.x)
            {
                temp = center.x - m_Max.x;
                distSquared += temp * temp;
            }
            if(center.y < m_Min.y)
            {
                temp = center.y - m_Min.y;
                distSquared += temp * temp;
            }
            else if(center.y > m_Max.y)
            {
                temp = center.y - m_Max.y;
                distSquared += temp * temp;
            }
            if(center.z < m_Min.z)
            {
                temp = center.z - m_Min.z;
                distSquared += temp * temp;
            }
            else if(center.z > m_Max.z)
            {
                temp = center.z - m_Max.z;
                distSquared += temp * temp;
            }

            float radius = sphere.GetRadius();
            if(distSquared > radius * radius)
                return OUTSIDE;
            else if(center.x - radius < m_Min.x || center.x + radius > m_Max.x || center.y - radius < m_Min.y || center.y + radius > m_Max.y || center.z - radius < m_Min.z || center.z + radius > m_Max.z)
                return INTERSECTS;
            else
                return INSIDE;
        }

        bool BoundingBox::IsInsideFast(const BoundingBox& box) const
        {
            LUMOS_PROFILE_FUNCTION();
            if(box.m_Max.x < m_Min.x || box.m_Min.x > m_Max.x || box.m_Max.y < m_Min.y || box.m_Min.y > m_Max.y || box.m_Max.z < m_Min.z || box.m_Min.z > m_Max.z)
                return false;
            else
                return true;
        }

        glm::vec3 BoundingBox::Size() const
        {
            return m_Max - m_Min;
        }

        glm::vec3 BoundingBox::Center() const
        {
            return (m_Max + m_Min) * 0.5f;
        }

        glm::vec3 BoundingBox::Min() const
        {
            return m_Min;
        }

        glm::vec3 BoundingBox::Max() const
        {
            return m_Max;
        }
    }
}
