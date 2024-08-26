#include "Precompiled.h"
#include "BoundingSphere.h"
#include "BoundingBox.h"
#include "Frustum.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Matrix4.h"

namespace Lumos
{
    namespace Maths
    {
        BoundingSphere::BoundingSphere()
        {
            m_Center = Vec3(0.0f);
            m_Radius = 0.0f;
        }

        BoundingSphere::BoundingSphere(const Vec3& center, float radius)
        {
            m_Center = center;
            m_Radius = radius;
        }

        BoundingSphere::BoundingSphere(const BoundingSphere& copy)
        {
            m_Center = copy.m_Center;
            m_Radius = copy.m_Radius;
        }

        BoundingSphere::BoundingSphere(const Vec3* points, unsigned int count)
        {
            if(count == 0)
            {
                m_Center = Vec3(0.0f);
                m_Radius = 0.0f;
                return;
            }

            m_Center = Vec3(0.0f);
            m_Radius = 0.0f;

            for(unsigned int i = 0; i < count; i++)
            {
                m_Center += points[i];
            }

            m_Center /= (float)count;

            float maxDist = 0.0f;
            for(unsigned int i = 0; i < count; i++)
            {
                float dist = Maths::Distance(points[i], m_Center);
                if(dist > maxDist)
                {
                    maxDist = dist;
                }
            }

            m_Radius = maxDist;
        }

        BoundingSphere::BoundingSphere(const Vec3* points, unsigned int count, const Vec3& center)
        {
            if(count == 0)
            {
                m_Center = Vec3(0.0f);
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
                float dist = Maths::Length(points[i] - m_Center);
                if(dist > maxDistSq)
                {
                    maxDistSq = dist;
                }
            }

            m_Radius = maxDistSq;
        }

        BoundingSphere::BoundingSphere(const Vec3* points, unsigned int count, const Vec3& center, float radius)
        {
            if(count == 0)
            {
                m_Center = Vec3(0.0f);
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
                float dist = Maths::Length(points[i] - m_Center);
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

        bool BoundingSphere::IsInside(const Vec3& point) const
        {
            return Maths::Length2(point - m_Center) <= m_Radius * m_Radius;
        }

        bool BoundingSphere::IsInside(const BoundingSphere& sphere) const
        {
            return Maths::Length2(sphere.m_Center - m_Center) <= (m_Radius + sphere.m_Radius) * (m_Radius + sphere.m_Radius);
        }

        bool BoundingSphere::IsInside(const BoundingBox& box) const
        {
            return box.IsInside(*this);
        }

        bool BoundingSphere::IsInside(const Frustum& frustum) const
        {
            return frustum.IsInside(*this);
        }

        const Vec3& BoundingSphere::GetCenter() const
        {
            return m_Center;
        }

        float BoundingSphere::GetRadius() const
        {
            return m_Radius;
        }

        void BoundingSphere::SetCenter(const Vec3& center)
        {
            m_Center = center;
        }

        void BoundingSphere::SetRadius(float radius)
        {
            m_Radius = radius;
        }

        bool BoundingSphere::Contains(const Vec3& point) const
        {
            return Maths::Length2(point - m_Center) <= m_Radius * m_Radius;
        }

        bool BoundingSphere::Contains(const BoundingSphere& other) const
        {
            return Maths::Distance2(other.m_Center, m_Center) <= (m_Radius + other.m_Radius) * (m_Radius + other.m_Radius);
        }
        bool BoundingSphere::Intersects(const BoundingSphere& other) const
        {
            return Maths::Distance2(other.m_Center, m_Center) <= (m_Radius + other.m_Radius) * (m_Radius + other.m_Radius);
        }
        bool BoundingSphere::Intersects(const Vec3& point) const
        {
            return Maths::Distance2(point, m_Center) <= m_Radius * m_Radius;
        }
        bool BoundingSphere::Intersects(const Vec3& point, float radius) const
        {
            return Maths::Distance2(point, m_Center) <= (m_Radius + radius) * (m_Radius + radius);
        }

        void BoundingSphere::Merge(const BoundingSphere& other)
        {
            float distance = Maths::Distance(other.m_Center, m_Center);

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

        void BoundingSphere::Merge(const Vec3& point)
        {
            float distance = Maths::Distance(point, m_Center);

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

        void BoundingSphere::Merge(const Vec3* points, unsigned int count)
        {
            if(count == 0)
                return;

            float radius = 0.0f;
            Vec3 center  = points[0];

            for(unsigned int i = 1; i < count; i++)
            {
                float distance = Maths::Distance(points[i], center);

                if(distance > radius)
                    radius = distance;

                center += points[i];
            }

            center /= (float)count;

            float distance = Maths::Distance(center, m_Center);

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

        void BoundingSphere::Transform(const Mat4& transform)
        {
            Vec3 center = transform * Vec4(m_Center, 1.0f);

            m_Center = center;
        }
    }
}
