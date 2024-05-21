#pragma once

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Lumos
{
    namespace Maths
    {
        struct WorldTransform
        {
            glm::mat4 WorldMatrix;
        };

        class Transform
        {
            template <typename Archive>
            friend void save(Archive& archive, const Transform& transform);

            template <typename Archive>
            friend void load(Archive& archive, Transform& transform);

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

        protected:
            glm::mat4 m_WorldMatrix;

            glm::vec3 m_LocalPosition;
            glm::vec3 m_LocalScale;
            glm::quat m_LocalOrientation;
        };
    }
}
