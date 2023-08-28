#pragma once

#include "Maths/MathsSerialisation.h"
#include <cereal/cereal.hpp>

namespace Lumos
{
    namespace Maths
    {
        class LUMOS_EXPORT Transform
        {
        public:
            Transform();
            Transform(const glm::mat4& matrix);
            Transform(const glm::vec3& position);
            ~Transform();

            void SetWorldMatrix(const glm::mat4& mat);

            void SetLocalTransform(const glm::mat4& localMat);

            void SetLocalPosition(const glm::vec3& localPos);
            void SetLocalScale(const glm::vec3& localScale);
            void SetLocalOrientation(const glm::quat& quat);

            const glm::mat4& GetWorldMatrix();
            glm::mat4 GetLocalMatrix();

            const glm::vec3 GetWorldPosition();
            const glm::quat GetWorldOrientation();

            const glm::vec3& GetLocalPosition() const;
            const glm::vec3& GetLocalScale() const;
            const glm::quat& GetLocalOrientation() const;

            // Updates Local Matrix from R,T and S vectors
            void UpdateMatrices();

            // Sets R,T and S vectors from Local Matrix
            void ApplyTransform();

            glm::vec3 GetUpDirection()
            {
                glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                up           = GetWorldOrientation() * up;
                return up;
            }

            glm::vec3 GetRightDirection()
            {
                glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
                right           = GetWorldOrientation() * right;
                return right;
            }

            glm::vec3 GetForwardDirection()
            {
                glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
                forward           = GetWorldOrientation() * forward;
                return forward;
            }

            template <typename Archive>
            void save(Archive& archive) const
            {
                archive(cereal::make_nvp("Position", m_LocalPosition), cereal::make_nvp("Rotation", m_LocalOrientation), cereal::make_nvp("Scale", m_LocalScale));
            }

            template <typename Archive>
            void load(Archive& archive)
            {
                archive(cereal::make_nvp("Position", m_LocalPosition), cereal::make_nvp("Rotation", m_LocalOrientation), cereal::make_nvp("Scale", m_LocalScale));
            }

        protected:
            glm::mat4 m_WorldMatrix;

            glm::vec3 m_LocalPosition;
            glm::vec3 m_LocalScale;
            glm::quat m_LocalOrientation;
        };
    }
}
