#include "Precompiled.h"
#include "SpringConstraint.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"

namespace Lumos
{
    SpringConstraint::SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, float springConstant, float dampingFactor)
        : m_pObj1(obj1)
        , m_pObj2(obj2)
        , m_springConstant(springConstant)
        , m_dampingFactor(dampingFactor)
    {
        Vec3 ab        = obj2->GetPosition() - obj1->GetPosition();
        m_restDistance = Maths::Length(ab);

        Vec3 r1    = (obj1->GetPosition() - m_pObj1->GetPosition());
        Vec3 r2    = (obj2->GetPosition() - m_pObj2->GetPosition());
        m_LocalOnA = Maths::Transpose(Mat3(m_pObj1->GetOrientation())) * r1;
        m_LocalOnB = Maths::Transpose(Mat3(m_pObj2->GetOrientation())) * r2;
    }

    SpringConstraint::SpringConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const Vec3& globalOnA, const Vec3& globalOnB, float springConstant, float dampingFactor)
        : m_pObj1(obj1)
        , m_pObj2(obj2)
        , m_springConstant(springConstant)
        , m_dampingFactor(dampingFactor)
    {
        Vec3 ab        = globalOnB - globalOnA;
        m_restDistance = Maths::Length(ab);

        Vec3 r1    = (globalOnA - m_pObj1->GetPosition());
        Vec3 r2    = (globalOnB - m_pObj2->GetPosition());
        m_LocalOnA = Maths::Transpose(Mat3(m_pObj1->GetOrientation())) * r1;
        m_LocalOnB = Maths::Transpose(Mat3(m_pObj2->GetOrientation())) * r2;
    }

    void SpringConstraint::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION();
        // UNIMPLEMENTED;

        if(m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass() == 0.0f)
            return;

        Vec3 r1 = Mat3(m_pObj1->GetOrientation()) * m_LocalOnA;
        Vec3 r2 = Mat3(m_pObj2->GetOrientation()) * m_LocalOnB;

        Vec3 globalOnA = r1 + m_pObj1->GetPosition();
        Vec3 globalOnB = r2 + m_pObj2->GetPosition();

        Vec3 ab  = globalOnB - globalOnA;
        Vec3 abn = ab.Normalised();

        Vec3 v0              = m_pObj1->GetLinearVelocity() + Maths::Cross(m_pObj1->GetAngularVelocity(), r1);
        Vec3 v1              = m_pObj2->GetLinearVelocity() + Maths::Cross(m_pObj2->GetAngularVelocity(), r2);
        float constraintMass = (m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass()) + Maths::Dot(abn, Maths::Cross(m_pObj1->GetInverseInertia() * Maths::Cross(r1, abn), r1) + Maths::Cross(m_pObj2->GetInverseInertia() * Maths::Cross(r2, abn), r2));

        float b = 0.0f;
        {
            float distanceOffset  = Maths::Length(ab) - m_restDistance;
            float baumgarteScalar = 0.1f;
            b                     = -(baumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * distanceOffset;
        }

        float jn = (-(Maths::Dot(v0 - v1, abn) + b) * m_springConstant) - (m_dampingFactor * Maths::Length((v0 - v1)));
        jn /= constraintMass;

        m_pObj1->SetLinearVelocity(m_pObj1->GetLinearVelocity() + abn * (jn * m_pObj1->GetInverseMass()));
        m_pObj2->SetLinearVelocity(m_pObj2->GetLinearVelocity() - abn * (jn * m_pObj2->GetInverseMass()));

        m_pObj1->SetAngularVelocity(m_pObj1->GetAngularVelocity() + m_pObj1->GetInverseInertia() * Maths::Cross(r1, abn * jn));
        m_pObj2->SetAngularVelocity(m_pObj2->GetAngularVelocity() - m_pObj2->GetInverseInertia() * Maths::Cross(r2, abn * jn));
    }

    void SpringConstraint::DebugDraw() const
    {
        Vec3 globalOnA = m_pObj1->GetOrientation() * m_LocalOnA + m_pObj1->GetPosition();
        Vec3 globalOnB = m_pObj2->GetOrientation() * m_LocalOnB + m_pObj2->GetPosition();

        DebugRenderer::DrawThickLine(globalOnA, globalOnB, 0.02f, true, Vec4(0.0f, 0.0f, 0.0f, 1.0f));
        DebugRenderer::DrawPoint(globalOnA, 0.05f, false, Vec4(1.0f, 0.8f, 1.0f, 1.0f));
        DebugRenderer::DrawPoint(globalOnB, 0.05f, false, Vec4(1.0f, 0.8f, 1.0f, 1.0f));
    }
}
