#include "Precompiled.h"
#include "Transform.h"
#include "Maths/Maths.h"

namespace Lumos
{
    namespace Maths
    {
        Transform::Transform()
        {
            m_LocalPosition = Vector3(0.0f, 0.0f, 0.0f);
            m_LocalOrientation = Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f);
            m_LocalScale = Vector3(1.0f, 1.0f, 1.0f);
            m_LocalMatrix = Matrix4();
            m_WorldMatrix = Matrix4();
        }

        Transform::Transform(const Matrix4& matrix)
        {
            m_LocalPosition = matrix.Translation();
            m_LocalOrientation = matrix.Rotation();
            m_LocalScale = matrix.Scale();
            m_LocalMatrix = matrix;
            m_WorldMatrix = matrix;
        }

        Transform::Transform(const Vector3& position)
        {
            m_LocalPosition = position;
            m_LocalOrientation = Quaternion::EulerAnglesToQuaternion(0.0f, 0.0f, 0.0f);
            m_LocalScale = Vector3(1.0f, 1.0f, 1.0f);
            m_LocalMatrix = Matrix4();
            m_WorldMatrix = Matrix4();
            SetLocalPosition(position);
        }

        Transform::~Transform() = default;

        void Transform::UpdateMatrices()
        {
            m_LocalMatrix = Matrix4::Translation(m_LocalPosition) * m_LocalOrientation.RotationMatrix4() * Matrix4::Scale(m_LocalScale);
            m_Dirty = false;
            m_HasUpdated = true;
        }

        void Transform::ApplyTransform()
        {
            m_LocalPosition = m_LocalMatrix.Translation();
            m_LocalOrientation = m_LocalMatrix.Rotation();
            m_LocalScale = m_LocalMatrix.Scale();
        }

        void Transform::SetWorldMatrix(const Matrix4& mat)
        {
            if(m_Dirty)
                UpdateMatrices();
            m_WorldMatrix = mat * m_LocalMatrix;
        }

        void Transform::SetLocalTransform(const Matrix4& localMat)
        {
            m_LocalMatrix = localMat;
            m_HasUpdated = true;

            ApplyTransform();
        }

        void Transform::SetLocalPosition(const Vector3& localPos)
        {
            m_Dirty = true;
            m_LocalPosition = localPos;
        }

        void Transform::SetLocalScale(const Vector3& newScale)
        {
            m_Dirty = true;
            m_LocalScale = newScale;
        }

        void Transform::SetLocalOrientation(const Quaternion& quat)
        {
            m_Dirty = true;
            m_LocalOrientation = quat;
        }

        const Matrix4& Transform::GetWorldMatrix()
        {
            if(m_Dirty)
                UpdateMatrices();

            return m_WorldMatrix;
        }

        const Matrix4& Transform::GetLocalMatrix()
        {
            if(m_Dirty)
                UpdateMatrices();

            return m_LocalMatrix;
        }

        const Vector3 Transform::GetWorldPosition() const
        {
            return m_WorldMatrix.Translation();
        }

        const Quaternion Transform::GetWorldOrientation() const
        {
            return m_WorldMatrix.Rotation();
        }

        const Vector3& Transform::GetLocalPosition() const
        {
            return m_LocalPosition;
        }

        const Vector3& Transform::GetLocalScale() const
        {
            return m_LocalScale;
        }

        const Quaternion& Transform::GetLocalOrientation() const
        {
            return m_LocalOrientation;
        }
    }
}
