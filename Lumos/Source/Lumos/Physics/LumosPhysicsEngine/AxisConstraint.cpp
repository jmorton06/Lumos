#include "Precompiled.h"
#include "Maths/Maths.h"
#include "AxisConstraint.h"
#include "RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    AxisConstraint::AxisConstraint(RigidBody3D* obj1, Axes axes)
        : m_pObj1(obj1)
        , m_Axes(axes)
    {
    }

    void AxisConstraint::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION();

        auto velocity = m_pObj1->GetAngularVelocity();
        velocity.x = 0.0f;
        velocity.z = 0.0f;

        switch(m_Axes)
        {
        case Axes::X:
            velocity.x = 0.0f;
            break;
        case Axes::Y:
            velocity.y = 0.0f;
            break;
        case Axes::Z:
            velocity.z = 0.0f;
            break;
        case Axes::XZ:
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            break;
        case Axes::XY:
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            break;
        case Axes::YZ:
            velocity.y = 0.0f;
            velocity.z = 0.0f;
            break;
        case Axes::XYZ:
            velocity.x = 0.0f;
            velocity.y = 0.0f;
            velocity.z = 0.0f;
            break;
        default:
            break;
        }

        m_pObj1->SetAngularVelocity(velocity);
    }

    void AxisConstraint::DebugDraw() const
    {
    }
}
