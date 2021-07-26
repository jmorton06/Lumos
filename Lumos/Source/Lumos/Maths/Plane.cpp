#include "Precompiled.h"
#include "Maths/Plane.h"

namespace Lumos::Maths
{
    // Static initialization order can not be relied on, so do not use Vector3 constants
    const Plane Plane::UP(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));

    void Plane::Transform(const Matrix3& transform)
    {
        Define(Matrix4(transform).Inverse().Transpose() * ToVector4());
    }

    void Plane::Transform(const Matrix4& transform)
    {
        Define(transform.Inverse().Transpose() * ToVector4());
    }

    Matrix4 Plane::ReflectionMatrix() const
    {
        //Maybe wrong
        return Matrix4(
            -2.0f * normal_.x * normal_.x + 1.0f,
            -2.0f * normal_.x * normal_.y,
            -2.0f * normal_.x * normal_.z,
            -2.0f * normal_.x * d_,
            -2.0f * normal_.y * normal_.x,
            -2.0f * normal_.y * normal_.y + 1.0f,
            -2.0f * normal_.y * normal_.z,
            -2.0f * normal_.y * d_,
            -2.0f * normal_.z * normal_.x,
            -2.0f * normal_.z * normal_.y,
            -2.0f * normal_.z * normal_.z + 1.0f,
            -2.0f * normal_.z * d_, 0.0f, 0.0f, 0.0f,1.0f);
    }

    Plane Plane::Transformed(const Matrix3& transform) const
    {
        return Plane(Matrix4(transform).Inverse().Transpose() * ToVector4());
    }

    Plane Plane::Transformed(const Matrix4& transform) const
    {
        return Plane(transform.Inverse().Transpose() * ToVector4());
    }
}
