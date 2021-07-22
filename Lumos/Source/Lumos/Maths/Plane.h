#pragma once
#include "Maths/Matrix4.h"

namespace Lumos::Maths
{
    /// Surface in three-dimensional space.
    class Plane
    {
    public:
        /// Construct a degenerate plane with zero normal and parameter.
        Plane() noexcept
            : d_(0.0f)
        {
        }

        /// Copy-construct from another plane.
        Plane(const Plane& plane) noexcept = default;

        /// Construct from 3 vertices.
        Plane(const Vector3& v0, const Vector3& v1, const Vector3& v2) noexcept
        {
            Define(v0, v1, v2);
        }

        /// Construct from a normal vector and a point on the plane.
        Plane(const Vector3& normal, const Vector3& point) noexcept
        {
            Define(normal, point);
        }

        Plane(const Vector3& normal, float distance) noexcept
        {
            normal_ = normal.Normalised();
            absNormal_ = normal_.Abs();
            d_ = distance;
        }

        /// Construct from a 4-dimensional vector, where the w coordinate is the plane parameter.
        explicit Plane(const Vector4& plane) noexcept
        {
            Define(plane);
        }

        /// Assign from another plane.
        Plane& operator=(const Plane& rhs) noexcept = default;

        /// Define from 3 vertices.
        void Define(const Vector3& v0, const Vector3& v1, const Vector3& v2)
        {
            Vector3 dist1 = v1 - v0;
            Vector3 dist2 = v2 - v0;

            Define(dist1.CrossProduct(dist2), v0);
        }

        /// Define from a normal vector and a point on the plane.
        void Define(const Vector3& normal, const Vector3& point)
        {
            normal_ = normal.Normalised();
            absNormal_ = normal_.Abs();
            d_ = -normal_.DotProduct(point);
        }

        /// Define from a 4-dimensional vector, where the w coordinate is the plane parameter.
        void Define(const Vector4& plane)
        {
            normal_ = Vector3(plane.x, plane.y, plane.z);
            absNormal_ = normal_.Abs();
            d_ = plane.w;
        }

        /// Transform with a 3x3 matrix.
        void Transform(const Matrix3& transform);
        /// Transform with a 4x4 matrix.
        void Transform(const Matrix4& transform);

        /// Project a point on the plane.
        Vector3 Project(const Vector3& point) const { return point - normal_ * (normal_.DotProduct(point) + d_); }

        /// Return signed distance to a point.
        float Distance(const Vector3& point) const { return normal_.DotProduct(point) + d_; }

        /// Reflect a Normalised direction vector.
        Vector3 Reflect(const Vector3& direction) const { return direction - (2.0f * normal_.DotProduct(direction) * normal_); }

        bool PointInPlane(const Vector3& position) const
        {
            return Vector3::Dot(position, normal_) + Distance(Maths::Vector3(0.0f)) >= -0.0001f;
        }

        /// Return a reflection matrix.
        Matrix4 ReflectionMatrix() const;
        /// Return transformed by a 3x3 matrix.
        Plane Transformed(const Matrix3& transform) const;
        /// Return transformed by a 4x4 matrix.
        Plane Transformed(const Matrix4& transform) const;

        /// Return as a vector.
        Vector4 ToVector4() const { return Vector4(normal_, d_); }

        /// Plane normal.
        Vector3 normal_;
        /// Plane absolute normal.
        Vector3 absNormal_;
        /// Plane constant.
        float d_ {};

        /// Plane at origin with normal pointing up.
        static const Plane UP;
    };
}
