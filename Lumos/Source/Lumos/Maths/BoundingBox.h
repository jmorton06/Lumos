#pragma once
#include "Maths/Rect.h"
#include "Maths/Vector3.h"

#ifdef LUMOS_SSE
#include <xmmintrin.h>
#endif

namespace Lumos::Maths
{
    class Polyhedron;
    class Frustum;
    class Matrix3;
    class Matrix4;
    class Sphere;

    /// Three-dimensional axis-aligned bounding box.
    class BoundingBox
    {
    public:
        /// Construct with zero size.
        BoundingBox() noexcept
            : min_(M_INFINITY, M_INFINITY, M_INFINITY)
            , max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
        {
        }

        /// Copy-construct from another bounding box.
        BoundingBox(const BoundingBox& box) noexcept
            : min_(box.min_)
            , max_(box.max_)
        {
        }

        /// Construct from a rect, with the Z dimension left zero.
        explicit BoundingBox(const Rect& rect) noexcept
            : min_(Vector3(rect.min_, 0.0f))
            , max_(Vector3(rect.max_, 0.0f))
        {
        }

        /// Construct from minimum and maximum vectors.
        BoundingBox(const Vector3& min, const Vector3& max) noexcept
            : min_(min)
            , max_(max)
        {
        }

        /// Construct from minimum and maximum floats (all dimensions same.)
        BoundingBox(float min, float max) noexcept
            : min_(Vector3(min, min, min))
            , max_(Vector3(max, max, max))
        {
        }

#ifdef LUMOS_SSE
        BoundingBox(__m128 min, __m128 max) noexcept
        {
            _mm_storeu_ps(&min_.x, min);
            _mm_storeu_ps(&max_.x, max);
        }
#endif

        /// Construct from an array of vertices.
        BoundingBox(const Vector3* vertices, unsigned count)
            : min_(M_INFINITY, M_INFINITY, M_INFINITY)
            , max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
        {
            Define(vertices, count);
        }

        /// Construct from a frustum.
        explicit BoundingBox(const Frustum& frustum)
            : min_(M_INFINITY, M_INFINITY, M_INFINITY)
            , max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
        {
            Define(frustum);
        }
        
        /// Construct from a sphere.
        explicit BoundingBox(const Sphere& sphere)
            : min_(M_INFINITY, M_INFINITY, M_INFINITY)
            , max_(-M_INFINITY, -M_INFINITY, -M_INFINITY)
        {
            Define(sphere);
        }

        /// Assign from another bounding box.
        BoundingBox& operator=(const BoundingBox& rhs) noexcept
        {
            min_ = rhs.min_;
            max_ = rhs.max_;
            return *this;
        }

        /// Assign from a Rect, with the Z dimension left zero.
        BoundingBox& operator=(const Rect& rhs) noexcept
        {
            min_ = Vector3(rhs.min_, 0.0f);
            max_ = Vector3(rhs.max_, 0.0f);
            return *this;
        }

        /// Test for equality with another bounding box.
        bool operator==(const BoundingBox& rhs) const { return (min_ == rhs.min_ && max_ == rhs.max_); }

        /// Test for inequality with another bounding box.
        bool operator!=(const BoundingBox& rhs) const { return (min_ != rhs.min_ || max_ != rhs.max_); }

        /// Define from another bounding box.
        void Define(const BoundingBox& box)
        {
            Define(box.min_, box.max_);
        }

        /// Define from a Rect.
        void Define(const Rect& rect)
        {
            Define(Vector3(rect.min_, 0.0f), Vector3(rect.max_, 0.0f));
        }

        /// Define from minimum and maximum vectors.
        void Define(const Vector3& min, const Vector3& max)
        {
            min_ = min;
            max_ = max;
        }

        /// Define from minimum and maximum floats (all dimensions same.)
        void Define(float min, float max)
        {
            min_ = Vector3(min, min, min);
            max_ = Vector3(max, max, max);
        }

        /// Define from a point.
        void Define(const Vector3& point)
        {
            min_ = max_ = point;
        }

        /// Merge a point.
        void Merge(const Vector3& point)
        {
#ifdef LUMOS_SSE
            __m128 vec = _mm_set_ps(1.f, point.z, point.y, point.x);
            _mm_storeu_ps(&min_.x, _mm_min_ps(_mm_loadu_ps(&min_.x), vec));
            _mm_storeu_ps(&max_.x, _mm_max_ps(_mm_loadu_ps(&max_.x), vec));
#else
            if(point.x < min_.x)
                min_.x = point.x;
            if(point.y < min_.y)
                min_.y = point.y;
            if(point.z < min_.z)
                min_.z = point.z;
            if(point.x > max_.x)
                max_.x = point.x;
            if(point.y > max_.y)
                max_.y = point.y;
            if(point.z > max_.z)
                max_.z = point.z;
#endif
        }

        /// Merge another bounding box.
        void Merge(const BoundingBox& box)
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&min_.x, _mm_min_ps(_mm_loadu_ps(&min_.x), _mm_loadu_ps(&box.min_.x)));
            _mm_storeu_ps(&max_.x, _mm_max_ps(_mm_loadu_ps(&max_.x), _mm_loadu_ps(&box.max_.x)));
#else
            if(box.min_.x < min_.x)
                min_.x = box.min_.x;
            if(box.min_.y < min_.y)
                min_.y = box.min_.y;
            if(box.min_.z < min_.z)
                min_.z = box.min_.z;
            if(box.max_.x > max_.x)
                max_.x = box.max_.x;
            if(box.max_.y > max_.y)
                max_.y = box.max_.y;
            if(box.max_.z > max_.z)
                max_.z = box.max_.z;
#endif
        }

        /// Define from an array of vertices.
        void Define(const Vector3* vertices, unsigned count);
        /// Define from a frustum.
        void Define(const Frustum& frustum);
        /// Define from a sphere.
        void Define(const Sphere& sphere);
        /// Merge an array of vertices.
        void Merge(const Vector3* vertices, unsigned count);
        /// Merge a frustum.
        void Merge(const Frustum& frustum);
        /// Merge a sphere.
        void Merge(const Sphere& sphere);
        /// Clip with another bounding box. The box can become degenerate (undefined) as a result.
        void Clip(const BoundingBox& box);
        /// Transform with a 3x3 matrix.
        void Transform(const Matrix3& transform);
        /// Transform with a 4x4 matrix.
        void Transform(const Matrix4& transform);

        /// Clear to undefined state.
        void Clear()
        {
#ifdef LUMOS_SSE
            _mm_storeu_ps(&min_.x, _mm_set1_ps(M_INFINITY));
            _mm_storeu_ps(&max_.x, _mm_set1_ps(-M_INFINITY));
#else
            min_ = Vector3(M_INFINITY, M_INFINITY, M_INFINITY);
            max_ = Vector3(-M_INFINITY, -M_INFINITY, -M_INFINITY);
#endif
        }

        /// Return true if this bounding box is defined via a previous call to Define() or Merge().
        bool Defined() const
        {
            return min_.x != M_INFINITY;
        }

        /// Return center.
        Vector3 Center() const { return (max_ + min_) * 0.5f; }

        /// Return size.
        Vector3 Size() const { return max_ - min_; }

        /// Return half-size.
        Vector3 HalfSize() const { return (max_ - min_) * 0.5f; }

        /// Return transformed by a 4x4 matrix.
        BoundingBox Transformed(const Matrix4& transform) const;
        /// Return transformed by a 3x3 matrix.
        BoundingBox Transformed(const Matrix3& transform) const;
        /// Return projected by a 4x4 projection matrix.
        Rect Projected(const Matrix4& projection) const;
        /// Return distance to point.
        float DistanceToPoint(const Vector3& point) const;

        /// Test if a point is inside.
        Intersection IsInside(const Vector3& point) const
        {
            if(point.x < min_.x || point.x > max_.x || point.y < min_.y || point.y > max_.y || point.z < min_.z || point.z > max_.z)
                return OUTSIDE;
            else
                return INSIDE;
        }

        /// Test if another bounding box is inside, outside or intersects.
        Intersection IsInside(const BoundingBox& box) const
        {
            if(box.max_.x < min_.x || box.min_.x > max_.x || box.max_.y < min_.y || box.min_.y > max_.y || box.max_.z < min_.z || box.min_.z > max_.z)
                return OUTSIDE;
            else if(box.min_.x < min_.x || box.max_.x > max_.x || box.min_.y < min_.y || box.max_.y > max_.y || box.min_.z < min_.z || box.max_.z > max_.z)
                return INTERSECTS;
            else
                return INSIDE;
        }

        /// Test if another bounding box is (partially) inside or outside.
        Intersection IsInsideFast(const BoundingBox& box) const
        {
            if(box.max_.x < min_.x || box.min_.x > max_.x || box.max_.y < min_.y || box.min_.y > max_.y || box.max_.z < min_.z || box.min_.z > max_.z)
                return OUTSIDE;
            else
                return INSIDE;
        }

        /// Test if a sphere is inside, outside or intersects.
        Intersection IsInside(const Sphere& sphere) const;
        /// Test if a sphere is (partially) inside or outside.
        Intersection IsInsideFast(const Sphere& sphere) const;

        /// Return as string.

        /// Minimum vector.
        Vector3 min_;
        float dummyMin_ {}; // This is never used, but exists to pad the min_ value to four floats.
        /// Maximum vector.
        Vector3 max_;
        float dummyMax_ {}; // This is never used, but exists to pad the max_ value to four floats.
    };
}
