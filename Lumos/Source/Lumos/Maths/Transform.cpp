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

        // Builds T * R * S directly instead of multiplying three matrices - this runs for
        // every transform in the scene, every frame.
        static Mat4 ComposeTRS(const Vec3& position, const Quat& orientation, const Vec3& scale)
        {
            Mat4 out = orientation.ToMatrix4();

            out.values[0] *= scale.x;
            out.values[1] *= scale.x;
            out.values[2] *= scale.x;
            out.values[4] *= scale.y;
            out.values[5] *= scale.y;
            out.values[6] *= scale.y;
            out.values[8] *= scale.z;
            out.values[9] *= scale.z;
            out.values[10] *= scale.z;

            out.values[12] = position.x;
            out.values[13] = position.y;
            out.values[14] = position.z;
            return out;
        }

        void Transform::SetWorldMatrix(const Mat4& mat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_WorldMatrix = mat * ComposeTRS(m_LocalPosition, m_LocalOrientation, m_LocalScale);
        }

        void Transform::SetWorldMatrix()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_WorldMatrix = ComposeTRS(m_LocalPosition, m_LocalOrientation, m_LocalScale);
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
            return ComposeTRS(m_LocalPosition, m_LocalOrientation, m_LocalScale);
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
