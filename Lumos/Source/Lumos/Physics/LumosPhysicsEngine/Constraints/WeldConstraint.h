#pragma once
#include "Constraint.h"
#include "Maths/Maths.h"

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

        glm::vec3 m_positionOffset;
        glm::quat m_orientation;
    };
}
