#pragma once

#include "Maths/Vector3.h"
#include "Maths/Matrix4.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
    namespace Maths
    {
        struct WorldTransform
        {
            Mat4 WorldMatrix;
        };

        class Transform
        {
            template <typename Archive>
            friend void save(Archive& archive, const Transform& transform);

            template <typename Archive>
            friend void load(Archive& archive, Transform& transform);

        public:
            Transform();
            Transform(const Mat4& matrix);
            Transform(const Vec3& position);
            ~Transform();

            void SetWorldMatrix(const Mat4& mat);
            void SetLocalTransform(const Mat4& localMat);

            void SetLocalPosition(const Vec3& localPos);
            void SetLocalScale(const Vec3& localScale);
            void SetLocalOrientation(const Quat& quat);

            const Mat4& GetWorldMatrix();
            Mat4 GetLocalMatrix();

            const Vec3 GetWorldPosition();
            const Quat GetWorldOrientation();

            const Vec3& GetLocalPosition() const;
            const Vec3& GetLocalScale() const;
            const Quat& GetLocalOrientation() const;

            Vec3 GetUpDirection()
            {
                Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
                up      = GetWorldOrientation() * up;
                return up;
            }

            Vec3 GetRightDirection()
            {
                Vec3 right = Vec3(1.0f, 0.0f, 0.0f);
                right      = GetWorldOrientation() * right;
                return right;
            }

            Vec3 GetForwardDirection()
            {
                Vec3 forward = Vec3(0.0f, 0.0f, -1.0f);
                forward      = GetWorldOrientation() * forward;
                return forward;
            }

        protected:
            Mat4 m_WorldMatrix;

            Vec3 m_LocalPosition;
            Vec3 m_LocalScale;
            Quat m_LocalOrientation;
        };
    }
}
