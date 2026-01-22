#pragma once
#include "Maths/Plane.h"
#include "Maths/Matrix4.h"

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
            Frustum(const Mat4& transform);
            Frustum(const Mat4& projection, const Mat4& view);
            ~Frustum();

            void Transform(const Mat4& transform);
            void Define(const Mat4& projection, const Mat4& view);
            void Define(const Mat4& transform);
            void DefineOrtho(float scale, float aspectRatio, float n, float f, const Mat4& viewMatrix);
            void Define(float fov, float aspectRatio, float n, float f, const Mat4& viewMatrix);

            bool IsInside(const Vec3& point) const;
            bool IsInside(const BoundingSphere& sphere) const;
            bool IsInside(const BoundingBox& box) const;
            bool IsInside(const Rect& rect) const;
            bool IsInside(const Plane& plane) const;
            bool IsInside(const Ray& ray) const;

            // SIMD-optimized versions
#ifdef LUMOS_SSE
            bool IsInsideFast(const BoundingSphere& sphere) const;
            bool IsInsideFast(const BoundingBox& box) const;
#endif

            const Plane& GetPlane(FrustumPlane plane) const;
            const Plane& GetPlane(int index) const { return m_Planes[index]; }
            Vec3* GetVerticies();

        private:
            void CalculateVertices(const Mat4& transform);

            Plane m_Planes[6];
            Vec3 m_Verticies[8];
        };
    }
}
