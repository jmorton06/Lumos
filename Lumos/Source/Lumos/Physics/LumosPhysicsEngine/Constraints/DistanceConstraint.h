#pragma once
#include "Constraint.h"

namespace Lumos
{
    namespace Maths
    {
        class Vector3;
    }

    class RigidBody3D;

    class LUMOS_EXPORT DistanceConstraint : public Constraint
    {
    public:
        DistanceConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const Maths::Vector3& globalOnA, const Maths::Vector3& globalOnB);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        float m_Distance;

        Maths::Vector3 m_LocalOnA;
        Maths::Vector3 m_LocalOnB;
    };
}