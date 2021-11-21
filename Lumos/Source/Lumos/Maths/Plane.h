#pragma once
#include <glm/mat4x4.hpp>

namespace Lumos
{
    class Plane
    {
    public:
        Plane();
        Plane(const glm::vec3& normal, float distance);
        Plane(const glm::vec3& point, const glm::vec3& normal);
        Plane(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3);
        Plane(const glm::vec4& plane);
        Plane(float a, float b, float c, float d);
        ~Plane();

        void Set(const glm::vec3& normal, float distance);
        void Set(const glm::vec3& point, const glm::vec3& normal);
        void Set(const glm::vec3& point1, const glm::vec3& point2, const glm::vec3& point3);
        void Set(const glm::vec4& plane);
        void SetNormal(const glm::vec3& normal);
        void SetDistance(float distance);
        void Transform(const glm::mat4& transform);
        Plane Transformed(const glm::mat4& transform) const;

        void Normalise();

        float Distance(const glm::vec3& point) const;
        float Distance(const glm::vec4& point) const;

        bool IsPointOnPlane(const glm::vec3& point) const;
        bool IsPointOnPlane(const glm::vec4& point) const;

        glm::vec3 Project(const glm::vec3& point) const { return point - Distance(point) * m_Normal; }

        inline glm::vec3 Normal() const { return m_Normal; }
        inline float Distance() const { return m_Distance; }

    private:
        glm::vec3 m_Normal;
        float m_Distance;
    };
}
