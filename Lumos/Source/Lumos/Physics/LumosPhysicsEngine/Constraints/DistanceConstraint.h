#pragma once
#include "Constraint.h"

namespace Lumos
{
    class RigidBody3D;

    class LUMOS_EXPORT DistanceConstraint : public Constraint
    {
    public:
        DistanceConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const Vec3& globalOnA, const Vec3& globalOnB);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        float m_Distance;

        Vec3 m_LocalOnA;
        Vec3 m_LocalOnB;
    };
}
