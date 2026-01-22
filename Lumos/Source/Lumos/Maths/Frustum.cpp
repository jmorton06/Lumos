#include "Precompiled.h"
#include "Maths/Frustum.h"
#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Rect.h"
#include "Maths/Ray.h"

#ifdef LUMOS_SSE
#include "Maths/SSEUtilities.h"
#include <smmintrin.h>
#endif

namespace Lumos
{
    namespace Maths
    {
        Frustum::Frustum()
        {
            Define(Mat4(1.0f));
        }

        Frustum::Frustum(const Mat4& transform)
        {
            Define(transform);
        }

        Frustum::Frustum(const Mat4& projection, const Mat4& view)
        {
            Mat4 m = projection * view;
            Define(m);
        }

        Frustum::~Frustum()
        {
        }

        void Frustum::Define(const Mat4& projection, const Mat4& view)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Mat4 m = projection * view;
            Define(m);
        }

        void Frustum::Transform(const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                m_Planes[i].Transform(transform);
            }

            for(int i = 0; i < 6; i++)
            {
                m_Planes[i].Normalise();
            }

            CalculateVertices(transform);
        }

        void Frustum::Define(const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            auto& m               = transform;
            m_Planes[PLANE_LEFT]  = Plane(m.Get(3, 0) + m.Get(0, 0), m.Get(3, 1) + m.Get(0, 1), m.Get(3, 2) + m.Get(0, 2), m.Get(3, 3) + m.Get(0, 3));
            m_Planes[PLANE_RIGHT] = Plane(m.Get(3, 0) - m.Get(0, 0), m.Get(3, 1) - m.Get(0, 1), m.Get(3, 2) - m.Get(0, 2), m.Get(3, 3) - m.Get(0, 3));
            m_Planes[PLANE_DOWN]  = Plane(m.Get(3, 0) + m.Get(1, 0), m.Get(3, 1) + m.Get(1, 1), m.Get(3, 2) + m.Get(1, 2), m.Get(3, 3) + m.Get(1, 3));
            m_Planes[PLANE_UP]    = Plane(m.Get(3, 0) - m.Get(1, 0), m.Get(3, 1) - m.Get(1, 1), m.Get(3, 2) - m.Get(1, 2), m.Get(3, 3) - m.Get(1, 3));
            m_Planes[PLANE_NEAR]  = Plane(m.Get(3, 0) + m.Get(2, 0), m.Get(3, 1) + m.Get(2, 1), m.Get(3, 2) + m.Get(2, 2), m.Get(3, 3) + m.Get(2, 3));
            m_Planes[PLANE_FAR]   = Plane(m.Get(3, 0) - m.Get(2, 0), m.Get(3, 1) - m.Get(2, 1), m.Get(3, 2) - m.Get(2, 2), m.Get(3, 3) - m.Get(2, 3));

            //            auto mtx_idx = [&](int col, int row) { return row + 4 * col; };
            //
            //            auto build_plane = [&](int row, int sign) {
            //                    return Plane(Vector3(m[mtx_idx(0, 3)] + sign * m[mtx_idx(0, row)], m[mtx_idx(1, 3)] + sign * m[mtx_idx(1, row)],
            //                                                             m[mtx_idx(2, 3)] + sign * m[mtx_idx(2, row)]),
            //                                             m[mtx_idx(3, 3)] + sign * m[mtx_idx(3, row)], true);
            //
            //            };
            //
            //            m_Planes[0] = build_plane(0, 1);
            //            m_Planes[1] = build_plane(0, -1);
            //            m_Planes[2] = build_plane(1, 1);
            //            m_Planes[3] = build_plane(1, -1);
            //            m_Planes[4] = build_plane(2, 1);
            //            m_Planes[5] = build_plane(2, -1);

            for(int i = 0; i < 6; i++)
            {
                m_Planes[i].Normalise();
            }

            CalculateVertices(transform);
        }

        void Frustum::DefineOrtho(float scale, float aspectRatio, float n, float f, const Mat4& viewMatrix)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Mat4 m = Matrix4::Orthographic(-scale * aspectRatio, scale * aspectRatio, -scale, scale, n, f);
            m      = m * viewMatrix;
            Define(m);
        }

        void Frustum::Define(float fov, float aspectRatio, float n, float f, const Mat4& viewMatrix)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            Mat4 m = Mat4::Perspective(n, f, aspectRatio, fov);
            m      = m * viewMatrix;
            Define(m);
        }

        bool Frustum::IsInside(const Vec3& point) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            // Test planes in order of likelihood to reject (early-out optimization)
            // Near and far planes reject most often, so test them first
            static const int planeOrder[6] = { PLANE_NEAR, PLANE_FAR, PLANE_LEFT, PLANE_RIGHT, PLANE_DOWN, PLANE_UP };

            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[planeOrder[i]].Distance(point) < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

        bool Frustum::IsInside(const BoundingSphere& sphere) const
        {
#ifdef LUMOS_SSE
            // Use SIMD-optimized version when available
            return IsInsideFast(sphere);
#else
            LUMOS_PROFILE_FUNCTION_LOW();

            // Test planes in order of likelihood to reject (early-out optimization)
            static const int planeOrder[6] = { PLANE_NEAR, PLANE_FAR, PLANE_LEFT, PLANE_RIGHT, PLANE_DOWN, PLANE_UP };

            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[planeOrder[i]].Distance(sphere.GetCenter()) < -sphere.GetRadius())
                {
                    return false;
                }
            }

            return true;
#endif
        }

        bool Frustum::IsInside(const BoundingBox& box) const
        {
#ifdef LUMOS_SSE
            // Use SIMD-optimized version when available
            return IsInsideFast(box);
#else
            LUMOS_PROFILE_FUNCTION_LOW();

            // Test planes in order of likelihood to reject (early-out optimization)
            static const int planeOrder[6] = { PLANE_NEAR, PLANE_FAR, PLANE_LEFT, PLANE_RIGHT, PLANE_DOWN, PLANE_UP };

            for(int i = 0; i < 6; i++)
            {
                int planeIdx = planeOrder[i];
                Vec3 p = box.Min(), n = box.Max();
                Vec3 N = m_Planes[planeIdx].Normal();
                if(N.x >= 0)
                {
                    p.x = box.Max().x;
                    n.x = box.Min().x;
                }
                if(N.y >= 0)
                {
                    p.y = box.Max().y;
                    n.y = box.Min().y;
                }
                if(N.z >= 0)
                {
                    p.z = box.Max().z;
                    n.z = box.Min().z;
                }

                if(m_Planes[planeIdx].Distance(p) < 0)
                {
                    return false;
                }
            }
            return true;
#endif
        }

        bool Frustum::IsInside(const Rect& rect) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                Vec3 N = m_Planes[i].Normal();
                if(N.x >= 0)
                {
                    if(m_Planes[i].Distance(Vec3(rect.GetPosition(), 0)) < 0)
                    {
                        return false;
                    }
                }
                else
                {
                    if(m_Planes[i].Distance(Vec3(rect.GetPosition().x + rect.GetSize().x, rect.GetPosition().y, 0)) < 0)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool Frustum::IsInside(const Ray& ray) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[i].Distance(ray.Origin) < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

        bool Frustum::IsInside(const Plane& plane) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[i].Distance(plane.Normal()) < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

        const Plane& Frustum::GetPlane(FrustumPlane plane) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Planes[plane];
        }

        Vec3* Frustum::GetVerticies()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Verticies;
        }

        void Frustum::CalculateVertices(const Mat4& transform)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            static const bool zerotoOne = false;

            Mat4 transformInv = transform.Inverse();

            m_Verticies[0] = Vec4(-1.0f, -1.0f, zerotoOne ? 0.0f : -1.0f, 1.0f);
            m_Verticies[1] = Vec4(1.0f, -1.0f, zerotoOne ? 0.0f : -1.0f, 1.0f);
            m_Verticies[2] = Vec4(1.0f, 1.0f, zerotoOne ? 0.0f : -1.0f, 1.0f);
            m_Verticies[3] = Vec4(-1.0f, 1.0f, zerotoOne ? 0.0f : -1.0f, 1.0f);

            m_Verticies[4] = Vec4(-1.0f, -1.0f, 1.0f, 1.0f);
            m_Verticies[5] = Vec4(1.0f, -1.0f, 1.0f, 1.0f);
            m_Verticies[6] = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
            m_Verticies[7] = Vec4(-1.0f, 1.0f, 1.0f, 1.0f);

            Vec4 temp;
            for(int i = 0; i < 8; i++)
            {
                temp           = transformInv * Vec4(m_Verticies[i], 1.0f);
                m_Verticies[i] = temp / temp.w;
            }
        }

#ifdef LUMOS_SSE
        // SIMD-optimized frustum culling tests
        // Tests all 6 planes simultaneously using SSE

        bool Frustum::IsInsideFast(const BoundingSphere& sphere) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            const Vec3& center = sphere.GetCenter();
            float radius = sphere.GetRadius();

            // Load sphere center into SIMD register
            __m128 centerX = _mm_set1_ps(center.x);
            __m128 centerY = _mm_set1_ps(center.y);
            __m128 centerZ = _mm_set1_ps(center.z);
            __m128 negRadius = _mm_set1_ps(-radius);

            // Test planes 0-3 (4 planes at once)
            {
                // Load plane normals and distances
                __m128 normalX = _mm_set_ps(m_Planes[3].Normal().x, m_Planes[2].Normal().x,
                                            m_Planes[1].Normal().x, m_Planes[0].Normal().x);
                __m128 normalY = _mm_set_ps(m_Planes[3].Normal().y, m_Planes[2].Normal().y,
                                            m_Planes[1].Normal().y, m_Planes[0].Normal().y);
                __m128 normalZ = _mm_set_ps(m_Planes[3].Normal().z, m_Planes[2].Normal().z,
                                            m_Planes[1].Normal().z, m_Planes[0].Normal().z);
                __m128 planeD = _mm_set_ps(m_Planes[3].Distance(), m_Planes[2].Distance(),
                                          m_Planes[1].Distance(), m_Planes[0].Distance());

                // Calculate distances: dot(normal, center) + d
                __m128 distances = _mm_mul_ps(normalX, centerX);
                distances = _mm_add_ps(distances, _mm_mul_ps(normalY, centerY));
                distances = _mm_add_ps(distances, _mm_mul_ps(normalZ, centerZ));
                distances = _mm_add_ps(distances, planeD);

                // Compare: distance < -radius (outside)
                __m128 outside = _mm_cmplt_ps(distances, negRadius);

                // If any plane rejects the sphere, it's outside
                if (_mm_movemask_ps(outside) != 0)
                    return false;
            }

            // Test planes 4-5 (remaining 2 planes)
            {
                __m128 normalX = _mm_set_ps(0.0f, 0.0f, m_Planes[5].Normal().x, m_Planes[4].Normal().x);
                __m128 normalY = _mm_set_ps(0.0f, 0.0f, m_Planes[5].Normal().y, m_Planes[4].Normal().y);
                __m128 normalZ = _mm_set_ps(0.0f, 0.0f, m_Planes[5].Normal().z, m_Planes[4].Normal().z);
                __m128 planeD = _mm_set_ps(0.0f, 0.0f, m_Planes[5].Distance(), m_Planes[4].Distance());

                __m128 distances = _mm_mul_ps(normalX, centerX);
                distances = _mm_add_ps(distances, _mm_mul_ps(normalY, centerY));
                distances = _mm_add_ps(distances, _mm_mul_ps(normalZ, centerZ));
                distances = _mm_add_ps(distances, planeD);

                __m128 outside = _mm_cmplt_ps(distances, negRadius);

                // Check only the first 2 lanes
                if ((_mm_movemask_ps(outside) & 0x3) != 0)
                    return false;
            }

            return true;
        }

        bool Frustum::IsInsideFast(const BoundingBox& box) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();

            // For each plane, find the positive vertex (furthest along plane normal)
            // If that vertex is outside, the whole box is outside

            Vec3 boxMin = box.Min();
            Vec3 boxMax = box.Max();

            // Load box min/max into SIMD registers
            __m128 minVec = _mm_set_ps(0.0f, boxMin.z, boxMin.y, boxMin.x);
            __m128 maxVec = _mm_set_ps(0.0f, boxMax.z, boxMax.y, boxMax.x);

            // Test each plane
            for (int i = 0; i < 6; i++)
            {
                const Vec3& normal = m_Planes[i].Normal();

                // Select positive vertex based on plane normal direction
                __m128 positiveVertex;
                if (normal.x >= 0)
                    positiveVertex = _mm_blend_ps(minVec, maxVec, 0x1); // x from max
                else
                    positiveVertex = _mm_blend_ps(maxVec, minVec, 0x1); // x from min

                if (normal.y >= 0)
                    positiveVertex = _mm_blend_ps(positiveVertex, maxVec, 0x2); // y from max
                else
                    positiveVertex = _mm_blend_ps(positiveVertex, minVec, 0x2); // y from min

                if (normal.z >= 0)
                    positiveVertex = _mm_blend_ps(positiveVertex, maxVec, 0x4); // z from max
                else
                    positiveVertex = _mm_blend_ps(positiveVertex, minVec, 0x4); // z from min

                // Calculate distance to plane
                __m128 planeNormal = _mm_set_ps(0.0f, normal.z, normal.y, normal.x);
                __m128 dotProduct = _mm_mul_ps(positiveVertex, planeNormal);

                // Horizontal add to get final dot product
                __m128 shuf = _mm_shuffle_ps(dotProduct, dotProduct, _MM_SHUFFLE(2, 3, 0, 1));
                __m128 sums = _mm_add_ps(dotProduct, shuf);
                shuf = _mm_movehl_ps(shuf, sums);
                sums = _mm_add_ss(sums, shuf);

                float distance = _mm_cvtss_f32(sums) + m_Planes[i].Distance();

                // If positive vertex is outside, whole box is outside
                if (distance < 0.0f)
                    return false;
            }

            return true;
        }
#endif
    }
}
