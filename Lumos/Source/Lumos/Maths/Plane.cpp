#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Maths/Plane.h"
#include "MathsUtilities.h"
namespace Lumos
{
    Plane::Plane()
    {
        m_Normal   = glm::vec3(0.0f, 1.0f, 0.0f);
        m_Distance = 0.0f;
    }

    Plane::Plane(const glm::vec3& normal, float distance)
    {
        m_Normal   = glm::normalize(normal);
        m_Distance = distance;
    }

    Plane::Plane(const glm::vec3& point, const glm::vec3& normal)
    {
        m_Normal   = glm::normalize(normal);
        m_Distance = glm::dot(normal, point);
    }

    Plane::Plane(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3)
    {
        glm::vec3 edge1 = point2 - point1;
        glm::vec3 edge2 = point3 - point1;
        m_Normal        = glm::normalize(glm::cross(edge1, edge2));
        m_Distance      = glm::dot(m_Normal, point1);
    }

    Plane::Plane(const glm::vec4& plane)
    {
        m_Normal   = glm::vec3(plane.x, plane.y, plane.z);
        m_Distance = plane.w;
    }

    Plane::Plane(float a, float b, float c, float d)
    {
        m_Normal   = glm::vec3(a, b, c);
        m_Distance = d;
    }

    Plane::~Plane()
    {
    }

    void Plane::Set(const glm::vec3& normal, float distance)
    {
        m_Normal   = normal;
        m_Distance = distance;
    }

    void Plane::Set(const glm::vec3& point, const glm::vec3& normal)
    {
        m_Normal   = normal;
        m_Distance = glm::dot(m_Normal, point);
    }

    void Plane::Set(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3)
    {
        glm::vec3 vec1 = point2 - point1;
        glm::vec3 vec2 = point3 - point1;
        m_Normal       = glm::cross(vec1, vec2);
        m_Normal       = glm::normalize(m_Normal);
        m_Distance     = glm::dot(m_Normal, point1);
    }

    void Plane::Set(const glm::vec4& plane)
    {
        m_Normal   = glm::vec3(plane.x, plane.y, plane.z);
        m_Distance = plane.w;
    }

    void Plane::Normalise()
    {
        float magnitude = glm::length(m_Normal);
        m_Normal /= magnitude;
        m_Distance /= magnitude;
    }

    float Plane::Distance(const glm::vec3& point) const
    {
        return glm::dot(point, m_Normal) + m_Distance;
    }

    float Plane::Distance(const glm::vec4& point) const
    {
        return glm::dot(glm::vec3(point), m_Normal) + m_Distance;
    }

    bool Plane::IsPointOnPlane(const glm::vec3& point) const
    {
        return Distance(point) >= -0.0001f;
    }

    bool Plane::IsPointOnPlane(const glm::vec4& point) const
    {
        return Distance(point) >= -Maths::M_EPSILON;
    }

    void Plane::SetNormal(const glm::vec3& normal)
    {
        m_Normal = normal;
    }

    void Plane::SetDistance(float distance)
    {
        m_Distance = distance;
    }

    void Plane::Transform(const glm::mat4& matrix)
    {
        glm::vec4 plane = glm::vec4(m_Normal, m_Distance);
        plane           = matrix * plane;
        m_Normal        = glm::vec3(plane.x, plane.y, plane.z);
        m_Distance      = plane.w;
    }

    Plane Plane::Transformed(const glm::mat4& matrix) const
    {
        glm::vec4 plane = glm::vec4(m_Normal, m_Distance);
        plane           = matrix * plane;
        return Plane(glm::vec3(plane.x, plane.y, plane.z), plane.w);
    }
}
