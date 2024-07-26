#pragma once
#include "Constraint.h"

#include "Maths/Vector3.h"
#include "Maths/Quaternion.h"

namespace Lumos
{
    class RigidBody3D;

    class LUMOS_EXPORT WeldConstraint : public Constraint
    {
    public:
        WeldConstraint(RigidBody3D* obj1, RigidBody3D* obj2);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;

    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        Vec3 m_positionOffset;
        Quat m_orientation;
    };
}
