#include "Precompiled.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"
#include "Rect.h"
#include "Maths/Matrix3.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        BoundingBox::BoundingBox()
        {
            m_Min = Vec3(FLT_MAX);
            m_Max = Vec3(-FLT_MAX);
        }

        BoundingBox::BoundingBox(const Vec3& min, const Vec3& max)
        {
            m_Min = min;
            m_Max = max;
        }

        BoundingBox::BoundingBox(const Rect& rect, const Vec3& center)
        {
            m_Min = center + Vec3(rect.GetPosition(), 0.0f);
            m_Max = center + Vec3(rect.GetPosition().x + rect.GetSize().x, rect.GetPosition().y + rect.GetSize().y, 0.0f);
        }

        BoundingBox::BoundingBox(const BoundingBox& other)
        {
            m_Min = other.m_Min;
            m_Max = other.m_Max;
        }

        BoundingBox::BoundingBox(const Vec3* points, uint32_t numPoints)
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

        void BoundingBox::Clear()
        {
            m_Min = Vec3(FLT_MAX);
            m_Max = Vec3(-FLT_MAX);
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

        void BoundingBox::Set(const Vec3& min, const Vec3& max)
        {
            m_Min = min;
            m_Max = max;
        }

        void BoundingBox::Set(const Vec3* points, uint32_t numPoints)
        {
            m_Min = Vec3(FLT_MAX);
            m_Max = Vec3(-FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                m_Min = Maths::Min(m_Min, points[i]);
                m_Max = Maths::Max(m_Max, points[i]);
            }
        }

        void BoundingBox::SetFromPoints(const Vec3* points, uint32_t numPoints)
        {
            m_Min = Vec3(FLT_MAX);
            m_Max = Vec3(-FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                m_Min = Maths::Min(m_Min, points[i]);
                m_Max = Maths::Max(m_Max, points[i]);
            }
        }

        void BoundingBox::SetFromPoints(const Vec3* points, uint32_t numPoints, const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
            Vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for(uint32_t i = 0; i < numPoints; i++)
            {
                Vec3 transformed = (transform * Vec4(points[i], 1.0f)).ToVector3();

                min.x = Maths::Min(min.x, transformed.x);
                min.y = Maths::Min(min.y, transformed.y);
                min.z = Maths::Min(min.z, transformed.z);

                max.x = Maths::Max(max.x, transformed.x);
                max.y = Maths::Max(max.y, transformed.y);
                max.z = Maths::Max(max.z, transformed.z);
            }

            m_Min = min;
            m_Max = max;
        }

        void BoundingBox::SetFromTransformedAABB(const BoundingBox& aabb, const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 min = aabb.m_Min;
            Vec3 max = aabb.m_Max;

            Vec3 minTransformed = transform * Vec4(min, 1.0f);
            Vec3 maxTransformed = transform * Vec4(max, 1.0f);

            m_Min = Maths::Min(minTransformed, maxTransformed);
            m_Max = Maths::Max(minTransformed, maxTransformed);
        }

        void BoundingBox::Translate(const Vec3& translation)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Min += translation;
            m_Max += translation;
        }

        void BoundingBox::Translate(float x, float y, float z)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Translate(Vec3(x, y, z));
        }

        void BoundingBox::Scale(const Vec3& scale)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Min *= scale;
            m_Max *= scale;
        }

        void BoundingBox::Scale(float x, float y, float z)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Min.x *= x;
            m_Min.y *= y;
            m_Min.z *= z;

            m_Max.x *= x;
            m_Max.y *= y;
            m_Max.z *= z;
        }

        void BoundingBox::Rotate(const Mat3& rotation)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 center  = Center();
            Vec3 extents = GetExtents();

            Vec3 rotatedExtents = rotation * extents;

            m_Min = center - rotatedExtents;
            m_Max = center + rotatedExtents;
        }

        void BoundingBox::Transform(const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 newCenter = transform * Vec4(Center(), 1.0f);
            Vec3 oldEdge   = Size() * 0.5f;
            Vec3 newEdge   = Vec3(
                Maths::Abs(transform.Get(0, 0)) * oldEdge.x + Maths::Abs(transform.Get(0, 1)) * oldEdge.y + Maths::Abs(transform.Get(0, 2)) * oldEdge.z,
                Maths::Abs(transform.Get(1, 0)) * oldEdge.x + Maths::Abs(transform.Get(1, 1)) * oldEdge.y + Maths::Abs(transform.Get(1, 2)) * oldEdge.z,
                Maths::Abs(transform.Get(2, 0)) * oldEdge.x + Maths::Abs(transform.Get(2, 1)) * oldEdge.y + Maths::Abs(transform.Get(2, 2)) * oldEdge.z);

            m_Min = newCenter - newEdge;
            m_Max = newCenter + newEdge;
        }

        BoundingBox BoundingBox::Transformed(const Mat4& transform) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            BoundingBox box(*this);
            box.Transform(transform);
            return box;
        }

        void BoundingBox::Merge(const BoundingBox& other)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
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

        void BoundingBox::Merge(const Vec3& point)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
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

        void BoundingBox::Merge(const BoundingBox& other, const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Merge(other.Transformed(transform));
        }
        void BoundingBox::Merge(const Vec3& point, const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 transformed = transform * Vec4(point, 1.0f);
            Merge(transformed);
        }

        void BoundingBox::Merge(const BoundingBox& other, const Mat3& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Merge(other.Transformed(Mat4(transform)));
        }
        void BoundingBox::Merge(const Vec3& point, const Mat3& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Vec3 transformed = transform * point;
            Merge(transformed);
        }

        void BoundingBox::ExtendToCube()
        {
            // Calculate the size of the current bounding box
            Vec3 size = Size();

            // Find the maximum dimension among x, y, and z
            float maxDimension = Maths::Max(Maths::Max(size.x, size.y), size.z);

            // Calculate the amount by which each dimension needs to be extended
            Vec3 extendAmount = Vec3(maxDimension - size.x, maxDimension - size.y, maxDimension - size.z) * 0.5f;

            // Update the bounding box to make it a cube
            m_Min -= extendAmount;
            m_Max += extendAmount;
        }

        Intersection BoundingBox::IsInside(const Vec3& point) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(point.x < m_Min.x || point.x > m_Max.x || point.y < m_Min.y || point.y > m_Max.y || point.z < m_Min.z || point.z > m_Max.z)
                return OUTSIDE;
            else
                return INSIDE;
        }

        Intersection BoundingBox::IsInside(const BoundingBox& box) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(box.m_Max.x < m_Min.x || box.m_Min.x > m_Max.x || box.m_Max.y < m_Min.y || box.m_Min.y > m_Max.y || box.m_Max.z < m_Min.z || box.m_Min.z > m_Max.z)
                return OUTSIDE;
            else if(box.m_Min.x < m_Min.x || box.m_Max.x > m_Max.x || box.m_Min.y < m_Min.y || box.m_Max.y > m_Max.y || box.m_Min.z < m_Min.z || box.m_Max.z > m_Max.z)
                return INTERSECTS;
            else
                return INSIDE;
        }

        Intersection BoundingBox::IsInside(const BoundingSphere& sphere) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            float distSquared = 0;
            float temp;
            const Vec3& center = sphere.GetCenter();

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
            LUMOS_PROFILE_FUNCTION_LOW();
            if(box.m_Max.x < m_Min.x || box.m_Min.x > m_Max.x || box.m_Max.y < m_Min.y || box.m_Min.y > m_Max.y || box.m_Max.z < m_Min.z || box.m_Min.z > m_Max.z)
                return false;
            else
                return true;
        }

        Vec3 BoundingBox::Size() const
        {
            return m_Max - m_Min;
        }

        Vec3 BoundingBox::Center() const
        {
            return (m_Max + m_Min) * 0.5f;
        }

        Vec3 BoundingBox::Min() const
        {
            return m_Min;
        }

        Vec3 BoundingBox::Max() const
        {
            return m_Max;
        }
    }
}
