#include "Precompiled.h"
#include "Maths/Frustum.h"
#include "Maths/BoundingBox.h"
#include "Maths/BoundingSphere.h"
#include "Maths/Rect.h"
#include "Maths/Ray.h"

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
            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[i].Distance(point) < 0.0f)
                {
                    return false;
                }
            }

            return true;
        }

        bool Frustum::IsInside(const BoundingSphere& sphere) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                if(m_Planes[i].Distance(sphere.GetCenter()) < -sphere.GetRadius())
                {
                    return false;
                }
            }

            return true;
        }

        bool Frustum::IsInside(const BoundingBox& box) const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(int i = 0; i < 6; i++)
            {
                Vec3 p = box.Min(), n = box.Max();
                Vec3 N = m_Planes[i].Normal();
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

                if(m_Planes[i].Distance(p) < 0)
                {
                    return false;
                }
            }
            return true;
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
    }
}
