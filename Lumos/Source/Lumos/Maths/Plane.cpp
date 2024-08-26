#include "Precompiled.h"
#include "Maths/Plane.h"
#include "MathsUtilities.h"
namespace Lumos
{
    Plane::Plane()
    {
        m_Normal   = Vec3(0.0f, 1.0f, 0.0f);
        m_Distance = 0.0f;
    }

    Plane::Plane(const Vec3& normal, float distance)
    {
        m_Normal   = normal.Normalised();
        m_Distance = distance;
    }

    Plane::Plane(const Vec3& point, const Vec3& normal)
    {
        m_Normal   = normal.Normalised();
        m_Distance = Maths::Dot(normal, point);
    }

    Plane::Plane(const Vec3& point1, const Vec3& point2, const Vec3& point3)
    {
        Vec3 edge1 = point2 - point1;
        Vec3 edge2 = point3 - point1;
        m_Normal   = Maths::Cross(edge1, edge2).Normalised();
        m_Distance = Maths::Dot(m_Normal, point1);
    }

    Plane::Plane(const Vec4& plane)
    {
        m_Normal   = Vec3(plane.x, plane.y, plane.z);
        m_Distance = plane.w;
    }

    Plane::Plane(float a, float b, float c, float d)
    {
        m_Normal   = Vec3(a, b, c);
        m_Distance = d;
    }

    Plane::~Plane()
    {
    }

    void Plane::Set(const Vec3& normal, float distance)
    {
        m_Normal   = normal;
        m_Distance = distance;
    }

    void Plane::Set(const Vec3& point, const Vec3& normal)
    {
        m_Normal   = normal;
        m_Distance = Maths::Dot(m_Normal, point);
    }

    void Plane::Set(const Vec3& point1, const Vec3& point2, const Vec3& point3)
    {
        Vec3 vec1 = point2 - point1;
        Vec3 vec2 = point3 - point1;
        m_Normal  = Maths::Cross(vec1, vec2);
        m_Normal.Normalise();
        m_Distance = Maths::Dot(m_Normal, point1);
    }

    void Plane::Set(const Vec4& plane)
    {
        m_Normal   = Vec3(plane.x, plane.y, plane.z);
        m_Distance = plane.w;
    }

    void Plane::Normalise()
    {
        float magnitude = Maths::Length(m_Normal);
        m_Normal /= magnitude;
        m_Distance /= magnitude;
    }

    float Plane::Distance(const Vec3& point) const
    {
        return Maths::Dot(point, m_Normal) + m_Distance;
    }

    float Plane::Distance(const Vec4& point) const
    {
        return Maths::Dot(Vec3(point), m_Normal) + m_Distance;
    }

    bool Plane::IsPointOnPlane(const Vec3& point) const
    {
        return Distance(point) >= -0.0001f;
    }

    bool Plane::IsPointOnPlane(const Vec4& point) const
    {
        return Distance(point) >= -Maths::M_EPSILON;
    }

    void Plane::SetNormal(const Vec3& normal)
    {
        m_Normal = normal;
    }

    void Plane::SetDistance(float distance)
    {
        m_Distance = distance;
    }

    void Plane::Transform(const Mat4& matrix)
    {
        Vec4 plane = Vec4(m_Normal, m_Distance);
        plane      = matrix * plane;
        m_Normal   = Vec3(plane.x, plane.y, plane.z);
        m_Distance = plane.w;
    }

    Plane Plane::Transformed(const Mat4& matrix) const
    {
        Vec4 plane = Vec4(m_Normal, m_Distance);
        plane      = matrix * plane;
        return Plane(Vec3(plane.x, plane.y, plane.z), plane.w);
    }
}
