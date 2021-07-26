#pragma once

#include "Constraint.h"

namespace Lumos
{
    class RigidBody3D;

    class LUMOS_EXPORT SpringConstraint : public Constraint
    {
    public:
        SpringConstraint(const SharedRef<RigidBody3D>& obj1, const SharedRef<RigidBody3D>& obj2, float springConstant, float dampingFactor);
        SpringConstraint(const SharedRef<RigidBody3D>& obj1, const SharedRef<RigidBody3D>& obj2, const Maths::Vector3& globalOnA, const Maths::Vector3& globalOnB, float springConstant, float dampingFactor);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        SharedRef<RigidBody3D> m_pObj1;
        SharedRef<RigidBody3D> m_pObj2;

        float m_restDistance;

        float m_springConstant;
        float m_dampingFactor;

        Maths::Vector3 m_LocalOnA;
        Maths::Vector3 m_LocalOnB;
    };
}