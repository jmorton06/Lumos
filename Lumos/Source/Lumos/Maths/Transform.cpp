#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Transform.h"
#include <glm/gtx/matrix_decompose.hpp>

namespace Lumos
{
    namespace Maths
    {
        Transform::Transform()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_LocalPosition    = glm::vec3(0.0f, 0.0f, 0.0f);
            m_LocalOrientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
            m_LocalScale       = glm::vec3(1.0f, 1.0f, 1.0f);
            // m_LocalMatrix      = glm::mat4(1.0f);
            m_WorldMatrix = glm::mat4(1.0f);
            // m_ParentMatrix     = glm::mat4(1.0f);
        }

        Transform::Transform(const glm::mat4& matrix)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(matrix, m_LocalScale, m_LocalOrientation, m_LocalPosition, skew, perspective);

            // m_LocalMatrix  = matrix;
            m_WorldMatrix = matrix;
            // m_ParentMatrix = glm::mat4(1.0f);
        }

        Transform::Transform(const glm::vec3& position)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_LocalPosition    = position;
            m_LocalOrientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
            m_LocalScale       = glm::vec3(1.0f, 1.0f, 1.0f);
            m_WorldMatrix      = glm::mat4(1.0f);
            SetLocalPosition(position);
        }

        Transform::~Transform() = default;

        void Transform::UpdateMatrices()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // m_LocalMatrix =

            // m_WorldMatrix = m_ParentMatrix * m_LocalMatrix;
        }

        void Transform::ApplyTransform()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // glm::vec3 skew;
            // glm::vec4 perspective;
            //  glm::decompose(m_LocalMatrix, m_LocalScale, m_LocalOrientation, m_LocalPosition, skew, perspective);
        }

        void Transform::SetWorldMatrix(const glm::mat4& mat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // if(m_Dirty)
            //   UpdateMatrices();
            // m_ParentMatrix = mat;
            m_WorldMatrix = mat * glm::translate(glm::mat4(1.0), m_LocalPosition) * glm::toMat4(m_LocalOrientation) * glm::scale(glm::mat4(1.0), m_LocalScale);
        }

        void Transform::SetLocalTransform(const glm::mat4& localMat)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // m_LocalMatrix = localMat;
            // m_HasUpdated  = true;

            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(localMat, m_LocalScale, m_LocalOrientation, m_LocalPosition, skew, perspective);

            ApplyTransform();

            // m_WorldMatrix = m_ParentMatrix * m_LocalMatrix;
        }

        void Transform::SetLocalPosition(const glm::vec3& localPos)
        {
            // m_Dirty         = true;
            m_LocalPosition = localPos;
        }

        void Transform::SetLocalScale(const glm::vec3& newScale)
        {
            // m_Dirty      = true;
            m_LocalScale = newScale;
        }

        void Transform::SetLocalOrientation(const glm::quat& quat)
        {
            // m_Dirty            = true;
            m_LocalOrientation = quat;
        }

        const glm::mat4& Transform::GetWorldMatrix()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // if(m_Dirty)
            // UpdateMatrices();

            return m_WorldMatrix;
        }

        glm::mat4 Transform::GetLocalMatrix()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            // if(m_Dirty)
            //   UpdateMatrices();

            return glm::translate(glm::mat4(1.0), m_LocalPosition) * glm::toMat4(m_LocalOrientation) * glm::scale(glm::mat4(1.0), m_LocalScale);
        }

        const glm::vec3 Transform::GetWorldPosition()
        {
            // if(m_Dirty)
            // UpdateMatrices();

            return m_WorldMatrix[3];
        }

        const glm::quat Transform::GetWorldOrientation()
        {
            // if(m_Dirty)
            // UpdateMatrices();

            return glm::toQuat(m_WorldMatrix);
        }

        const glm::vec3& Transform::GetLocalPosition() const
        {
            return m_LocalPosition;
        }

        const glm::vec3& Transform::GetLocalScale() const
        {
            return m_LocalScale;
        }

        const glm::quat& Transform::GetLocalOrientation() const
        {
            return m_LocalOrientation;
        }
    }
}
