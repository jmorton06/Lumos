#pragma once
#include "Constraint.h"
#include <glm/ext/vector_float3.hpp>

namespace Lumos
{
    class RigidBody3D;

    class LUMOS_EXPORT SpringConstraint : public Constraint
    {
    public:
        SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, float springConstant, float dampingFactor);
        SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const glm::vec3& globalOnA, const glm::vec3& globalOnB, float springConstant, float dampingFactor);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        float m_restDistance;

        float m_springConstant;
        float m_dampingFactor;

        glm::vec3 m_LocalOnA;
        glm::vec3 m_LocalOnB;
    };
}
