#pragma once
#include "Maths/Plane.h"
#include <glm/mat4x4.hpp>

namespace Lumos
{
    namespace Maths
    {
        class Ray;
        class Rect;
        class BoundingBox;
        class BoundingSphere;

        enum FrustumPlane
        {
            PLANE_NEAR = 0,
            PLANE_LEFT,
            PLANE_RIGHT,
            PLANE_UP,
            PLANE_DOWN,
            PLANE_FAR,
        };

        class Frustum
        {
        public:
            Frustum();
            Frustum(const glm::mat4& transform);
            Frustum(const glm::mat4& projection, const glm::mat4& view);
            ~Frustum();

            void Transform(const glm::mat4& transform);
            void Define(const glm::mat4& projection, const glm::mat4& view);
            void Define(const glm::mat4& transform);
            void DefineOrtho(float scale, float aspectRatio, float n, float f, const glm::mat4& viewMatrix);
            void Define(float fov, float aspectRatio, float n, float f, const glm::mat4& viewMatrix);

            bool IsInside(const glm::vec3& point) const;
            bool IsInside(const BoundingSphere& sphere) const;
            bool IsInside(const BoundingBox& box) const;
            bool IsInside(const Rect& rect) const;
            bool IsInside(const Plane& plane) const;
            bool IsInside(const Ray& ray) const;

            const Plane& GetPlane(FrustumPlane plane) const;
            const Plane& GetPlane(int index) const { return m_Planes[index]; }
            glm::vec3* GetVerticies();

        private:
            void CalculateVertices(const glm::mat4& transform);

            Plane m_Planes[6];
            glm::vec3 m_Verticies[8];
        };
    }
}
