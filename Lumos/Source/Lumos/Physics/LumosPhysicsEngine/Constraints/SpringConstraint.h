#pragma once
#include "Constraint.h"
#include "Maths/Vector3.h"

namespace Lumos
{
    class RigidBody3D;

    class LUMOS_EXPORT SpringConstraint : public Constraint
    {
    public:
        SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, float springConstant, float dampingFactor);
        SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const Vec3& globalOnA, const Vec3& globalOnB, float springConstant, float dampingFactor);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        float m_restDistance;

        float m_springConstant;
        float m_dampingFactor;

        Vec3 m_LocalOnA;
        Vec3 m_LocalOnB;
    };
}
