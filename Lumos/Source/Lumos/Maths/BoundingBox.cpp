#include "Precompiled.h"

#include "Maths/Frustum.h"
#include "Maths/Matrix4.h"

namespace Lumos::Maths
{
    void BoundingBox::Define(const Vector3* vertices, unsigned count)
    {
        Clear();

        if(!count)
            return;

        Merge(vertices, count);
    }

    void BoundingBox::Define(const Frustum& frustum)
    {
        Clear();
        Define(frustum.vertices_, NUM_FRUSTUM_VERTICES);
    }

    void BoundingBox::Define(const Sphere& sphere)
    {
        const Vector3& center = sphere.center_;
        float radius = sphere.radius_;

        min_ = center + Vector3(-radius, -radius, -radius);
        max_ = center + Vector3(radius, radius, radius);
    }

    void BoundingBox::Merge(const Vector3* vertices, unsigned count)
    {
        while(count--)
            Merge(*vertices++);
    }

    void BoundingBox::Merge(const Frustum& frustum)
    {
        Merge(frustum.vertices_, NUM_FRUSTUM_VERTICES);
    }

    void BoundingBox::Merge(const Sphere& sphere)
    {
        const Vector3& center = sphere.center_;
        float radius = sphere.radius_;

        Merge(center + Vector3(radius, radius, radius));
        Merge(center + Vector3(-radius, -radius, -radius));
    }

    void BoundingBox::Clip(const BoundingBox& box)
    {
        if(box.min_.x > min_.x)
            min_.x = box.min_.x;
        if(box.max_.x < max_.x)
            max_.x = box.max_.x;
        if(box.min_.y > min_.y)
            min_.y = box.min_.y;
        if(box.max_.y < max_.y)
            max_.y = box.max_.y;
        if(box.min_.z > min_.z)
            min_.z = box.min_.z;
        if(box.max_.z < max_.z)
            max_.z = box.max_.z;

        if(min_.x > max_.x || min_.y > max_.y || min_.z > max_.z)
        {
            min_ = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
            max_ = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);
        }
    }

    void BoundingBox::Transform(const Matrix3& transform)
    {
        *this = Transformed(Matrix4(transform));
    }

    void BoundingBox::Transform(const Matrix4& transform)
    {
        *this = Transformed(transform);
    }

    BoundingBox BoundingBox::Transformed(const Matrix3& transform) const
    {
        return Transformed(Matrix4(transform));
    }

    BoundingBox BoundingBox::Transformed(const Matrix4& transform) const
    {
        LUMOS_PROFILE_FUNCTION();
#ifdef LUMOS_SSE
        const __m128 one = _mm_set_ss(1.f);
        __m128 minPt = _mm_movelh_ps(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)&min_.x), _mm_unpacklo_ps(_mm_set_ss(min_.z), one));
        __m128 maxPt = _mm_movelh_ps(_mm_loadl_pi(_mm_setzero_ps(), (const __m64*)&max_.x), _mm_unpacklo_ps(_mm_set_ss(max_.z), one));
        __m128 centerPoint = _mm_mul_ps(_mm_add_ps(minPt, maxPt), _mm_set1_ps(0.5f));
        __m128 halfSize = _mm_sub_ps(centerPoint, minPt);
        __m128 m0 = _mm_loadu_ps(&transform.m00_);
        __m128 m1 = _mm_loadu_ps(&transform.m10_);
        __m128 m2 = _mm_loadu_ps(&transform.m20_);
        __m128 r0 = _mm_mul_ps(m0, centerPoint);
        __m128 r1 = _mm_mul_ps(m1, centerPoint);
        __m128 t0 = _mm_add_ps(_mm_unpacklo_ps(r0, r1), _mm_unpackhi_ps(r0, r1));
        __m128 r2 = _mm_mul_ps(m2, centerPoint);
        const __m128 zero = _mm_setzero_ps();
        __m128 t2 = _mm_add_ps(_mm_unpacklo_ps(r2, zero), _mm_unpackhi_ps(r2, zero));
        __m128 newCenter = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));
        const __m128 absMask = _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF));
        __m128 x = _mm_and_ps(absMask, _mm_mul_ps(m0, halfSize));
        __m128 y = _mm_and_ps(absMask, _mm_mul_ps(m1, halfSize));
        __m128 z = _mm_and_ps(absMask, _mm_mul_ps(m2, halfSize));
        t0 = _mm_add_ps(_mm_unpacklo_ps(x, y), _mm_unpackhi_ps(x, y));
        t2 = _mm_add_ps(_mm_unpacklo_ps(z, zero), _mm_unpackhi_ps(z, zero));
        __m128 newDir = _mm_add_ps(_mm_movelh_ps(t0, t2), _mm_movehl_ps(t2, t0));
        return BoundingBox(_mm_sub_ps(newCenter, newDir), _mm_add_ps(newCenter, newDir));
#else
        Vector3 newCenter = transform * Center();
        Vector3 oldEdge = Size() * 0.5f;
        Vector3 newEdge = Vector3(
            Abs(transform.m00_) * oldEdge.x + Abs(transform.m01_) * oldEdge.y + Abs(transform.m02_) * oldEdge.z,
            Abs(transform.m10_) * oldEdge.x + Abs(transform.m11_) * oldEdge.y + Abs(transform.m12_) * oldEdge.z,
            Abs(transform.m20_) * oldEdge.x + Abs(transform.m21_) * oldEdge.y + Abs(transform.m22_) * oldEdge.z);

        return BoundingBox(newCenter - newEdge, newCenter + newEdge);
#endif
    }

    Rect BoundingBox::Projected(const Matrix4& projection) const
    {
        LUMOS_PROFILE_FUNCTION();
        Vector3 projMin = min_;
        Vector3 projMax = max_;
        if(projMin.z < M_MIN_NEARCLIP)
            projMin.z = M_MIN_NEARCLIP;
        if(projMax.z < M_MIN_NEARCLIP)
            projMax.z = M_MIN_NEARCLIP;

        Vector3 vertices[8];
        vertices[0] = projMin;
        vertices[1] = Vector3(projMax.x, projMin.y, projMin.z);
        vertices[2] = Vector3(projMin.x, projMax.y, projMin.z);
        vertices[3] = Vector3(projMax.x, projMax.y, projMin.z);
        vertices[4] = Vector3(projMin.x, projMin.y, projMax.z);
        vertices[5] = Vector3(projMax.x, projMin.y, projMax.z);
        vertices[6] = Vector3(projMin.x, projMax.y, projMax.z);
        vertices[7] = projMax;

        Rect rect;
        for(const auto& vertice : vertices)
        {
            Vector3 projected = projection * vertice;
            rect.Merge(Vector2(projected.x, projected.y));
        }

        return rect;
    }

    float BoundingBox::DistanceToPoint(const Vector3& point) const
    {
        LUMOS_PROFILE_FUNCTION();
        const Vector3 offset = Center() - point;
        const Vector3 absOffset(Abs(offset.x), Abs(offset.y), Abs(offset.z));
        return VectorMax(Vector3::ZERO, absOffset - HalfSize()).Length();
    }

    Intersection BoundingBox::IsInside(const Sphere& sphere) const
    {
        LUMOS_PROFILE_FUNCTION();
        float distSquared = 0;
        float temp;
        const Vector3& center = sphere.center_;

        if(center.x < min_.x)
        {
            temp = center.x - min_.x;
            distSquared += temp * temp;
        }
        else if(center.x > max_.x)
        {
            temp = center.x - max_.x;
            distSquared += temp * temp;
        }
        if(center.y < min_.y)
        {
            temp = center.y - min_.y;
            distSquared += temp * temp;
        }
        else if(center.y > max_.y)
        {
            temp = center.y - max_.y;
            distSquared += temp * temp;
        }
        if(center.z < min_.z)
        {
            temp = center.z - min_.z;
            distSquared += temp * temp;
        }
        else if(center.z > max_.z)
        {
            temp = center.z - max_.z;
            distSquared += temp * temp;
        }

        float radius = sphere.radius_;
        if(distSquared > radius * radius)
            return OUTSIDE;
        else if(center.x - radius < min_.x || center.x + radius > max_.x || center.y - radius < min_.y || center.y + radius > max_.y || center.z - radius < min_.z || center.z + radius > max_.z)
            return INTERSECTS;
        else
            return INSIDE;
    }

    Intersection BoundingBox::IsInsideFast(const Sphere& sphere) const
    {
        LUMOS_PROFILE_FUNCTION();
        float distSquared = 0;
        float temp;
        const Vector3& center = sphere.center_;

        if(center.x < min_.x)
        {
            temp = center.x - min_.x;
            distSquared += temp * temp;
        }
        else if(center.x > max_.x)
        {
            temp = center.x - max_.x;
            distSquared += temp * temp;
        }
        if(center.y < min_.y)
        {
            temp = center.y - min_.y;
            distSquared += temp * temp;
        }
        else if(center.y > max_.y)
        {
            temp = center.y - max_.y;
            distSquared += temp * temp;
        }
        if(center.z < min_.z)
        {
            temp = center.z - min_.z;
            distSquared += temp * temp;
        }
        else if(center.z > max_.z)
        {
            temp = center.z - max_.z;
            distSquared += temp * temp;
        }

        float radius = sphere.radius_;
        if(distSquared >= radius * radius)
            return OUTSIDE;
        else
            return INSIDE;
    }
}
