#pragma once
#include "Maths/Matrix4.h"
#include "Vector3.h"
#include "MathsFwd.h"

namespace Lumos
{
    class Plane
    {
    public:
        Plane();
        Plane(const Vec3& normal, float distance);
        Plane(const Vec3& point, const Vec3& normal);
        Plane(const Vec3& point1, const Vec3& point2, const Vec3& point3);
        Plane(const Vec4& plane);
        Plane(float a, float b, float c, float d);
        ~Plane();

        void Set(const Vec3& normal, float distance);
        void Set(const Vec3& point, const Vec3& normal);
        void Set(const Vec3& point1, const Vec3& point2, const Vec3& point3);
        void Set(const Vec4& plane);
        void SetNormal(const Vec3& normal);
        void SetDistance(float distance);
        void Transform(const Mat4& transform);
        Plane Transformed(const Mat4& transform) const;

        void Normalise();

        float Distance(const Vec3& point) const;
        float Distance(const Vec4& point) const;

        bool IsPointOnPlane(const Vec3& point) const;
        bool IsPointOnPlane(const Vec4& point) const;

        Vec3 Project(const Vec3& point) const { return point - Distance(point) * m_Normal; }

        inline Vec3 Normal() const { return m_Normal; }
        inline float Distance() const { return m_Distance; }

    private:
        Vec3 m_Normal;
        float m_Distance;
    };
}
