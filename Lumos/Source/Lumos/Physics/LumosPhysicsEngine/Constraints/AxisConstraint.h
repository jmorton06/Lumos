#pragma once

#include "Constraint.h"

namespace Lumos
{
    class RigidBody3D;

    enum class Axes
    {
        X = 0,
        Y,
        Z,
        XY,
        XZ,
        YZ,
        XYZ
    };

    class LUMOS_EXPORT AxisConstraint : public Constraint
    {
    public:
        AxisConstraint(RigidBody3D* obj1, Axes axes);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;
        Axes GetAxes() { return m_Axes; }

    protected:
        RigidBody3D* m_pObj1;
        Axes m_Axes;
    };
}
