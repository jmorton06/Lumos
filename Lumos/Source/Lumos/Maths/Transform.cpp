#include "Precompiled.h"
#include "Transform.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    namespace Maths
    {
        Transform::Transform()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_LocalPosition    = Vec3(0.0f, 0.0f, 0.0f);
            m_LocalOrientation = Quat(Vec3(0.0f, 0.0f, 0.0f));
            m_LocalScale       = Vec3(1.0f, 1.0f, 1.0f);
            m_WorldMatrix      = Mat4(1.0f);
        }

        Transform::Transform(const Mat4& matrix)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_WorldMatrix = Mat4(1.0f);
            matrix.Decompose(m_LocalPosition, m_LocalOrientation, m_LocalScale);
        }

        Transform::Transform(const Vec3& position)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_LocalPosition    = position;
            m_LocalOrientation = Quat(Vec3(0.0f, 0.0f, 0.0f));
            m_LocalScale       = Vec3(1.0f, 1.0f, 1.0f);
            m_WorldMatrix      = Mat4(1.0f);
        }

        Transform::~Transform() = default;

        void Transform::SetWorldMatrix(const Mat4& mat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_WorldMatrix = mat * Mat4::Translation(m_LocalPosition) * Maths::ToMat4(m_LocalOrientation) * Mat4::Scale(m_LocalScale);
        }

        void Transform::SetLocalTransform(const Mat4& localMat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            localMat.Decompose(m_LocalPosition, m_LocalOrientation, m_LocalScale);
        }

        void Transform::SetLocalPosition(const Vec3& localPos)
        {
            m_LocalPosition = localPos;
        }

        void Transform::SetLocalScale(const Vec3& newScale)
        {
            m_LocalScale = newScale;
        }

        void Transform::SetLocalOrientation(const Quat& quat)
        {
            m_LocalOrientation = quat;
        }

        const Mat4& Transform::GetWorldMatrix()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_WorldMatrix;
        }

        Mat4 Transform::GetLocalMatrix()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return Mat4::Translation(m_LocalPosition) * Maths::ToMat4(m_LocalOrientation) * Mat4::Scale(m_LocalScale);
        }

        const Vec3 Transform::GetWorldPosition()
        {
            return m_WorldMatrix.Translation();
        }

        const Quat Transform::GetWorldOrientation()
        {
            return m_WorldMatrix.Rotation();
        }

        const Vec3 Transform::GetWorldScale()
        {
            return m_WorldMatrix.Scale();
        }

        const Vec3& Transform::GetLocalPosition() const
        {
            return m_LocalPosition;
        }

        const Vec3& Transform::GetLocalScale() const
        {
            return m_LocalScale;
        }

        const Quat& Transform::GetLocalOrientation() const
        {
            return m_LocalOrientation;
        }
    }
}
