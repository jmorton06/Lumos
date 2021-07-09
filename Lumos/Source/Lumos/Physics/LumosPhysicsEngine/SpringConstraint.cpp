#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "SpringConstraint.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{
    SpringConstraint::SpringConstraint(const SharedRef<RigidBody3D>& obj1, const SharedRef<RigidBody3D>& obj2, float springConstant, float dampingFactor)
        : m_pObj1(obj1)
        , m_pObj2(obj2)
        , m_springConstant(springConstant)
        , m_dampingFactor(dampingFactor)
    {
        Maths::Vector3 ab = obj2->GetPosition() - obj1->GetPosition();
        m_restDistance = ab.Length();

        Maths::Vector3 r1 = (obj1->GetPosition() - m_pObj1->GetPosition());
        Maths::Vector3 r2 = (obj2->GetPosition() - m_pObj2->GetPosition());
        m_LocalOnA = Maths::Matrix3::Transpose(m_pObj1->GetOrientation().RotationMatrix()) * r1;
        m_LocalOnB = Maths::Matrix3::Transpose(m_pObj2->GetOrientation().RotationMatrix()) * r2;
    }

    SpringConstraint::SpringConstraint(const SharedRef<RigidBody3D>& obj1, const SharedRef<RigidBody3D>& obj2, const Maths::Vector3& globalOnA, const Maths::Vector3& globalOnB, float springConstant, float dampingFactor)
        : m_pObj1(obj1)
        , m_pObj2(obj2)
        , m_springConstant(springConstant)
        , m_dampingFactor(dampingFactor)
    {
        Maths::Vector3 ab = globalOnB - globalOnA;
        m_restDistance = ab.Length();

        Maths::Vector3 r1 = (globalOnA - m_pObj1->GetPosition());
        Maths::Vector3 r2 = (globalOnB - m_pObj2->GetPosition());
        m_LocalOnA = Maths::Matrix3::Transpose(m_pObj1->GetOrientation().RotationMatrix()) * r1;
        m_LocalOnB = Maths::Matrix3::Transpose(m_pObj2->GetOrientation().RotationMatrix()) * r2;
    }

    void SpringConstraint::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass() == 0.0f)
            return;

        Maths::Vector3 r1 = m_pObj1->GetOrientation().RotationMatrix() * m_LocalOnA;
        Maths::Vector3 r2 = m_pObj2->GetOrientation().RotationMatrix() * m_LocalOnB;

        Maths::Vector3 globalOnA = r1 + m_pObj1->GetPosition();
        Maths::Vector3 globalOnB = r2 + m_pObj2->GetPosition();

        Maths::Vector3 ab = globalOnB - globalOnA;
        Maths::Vector3 abn = ab;
        abn.Normalise();

        Maths::Vector3 v0 = m_pObj1->GetLinearVelocity() + Maths::Vector3::Cross(m_pObj1->GetAngularVelocity(), r1);
        Maths::Vector3 v1 = m_pObj2->GetLinearVelocity() + Maths::Vector3::Cross(m_pObj2->GetAngularVelocity(), r2);
        float constraintMass = (m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass()) + Maths::Vector3::Dot(abn, Maths::Vector3::Cross(m_pObj1->GetInverseInertia() * Maths::Vector3::Cross(r1, abn), r1) + Maths::Vector3::Cross(m_pObj2->GetInverseInertia() * Maths::Vector3::Cross(r2, abn), r2));

        float b = 0.0f;
        {
            float distanceOffset = ab.Length() - m_restDistance;
            float baumgarteScalar = 0.1f;
            b = -(baumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * distanceOffset;
        }

        float jn = (-(Maths::Vector3::Dot(v0 - v1, abn) + b) * m_springConstant) - (m_dampingFactor * (v0 - v1).Length());
        jn /= constraintMass;

        m_pObj1->SetLinearVelocity(m_pObj1->GetLinearVelocity() + abn * (jn * m_pObj1->GetInverseMass()));
        m_pObj2->SetLinearVelocity(m_pObj2->GetLinearVelocity() - abn * (jn * m_pObj2->GetInverseMass()));

        m_pObj1->SetAngularVelocity(m_pObj1->GetAngularVelocity() + m_pObj1->GetInverseInertia() * Maths::Vector3::Cross(r1, abn * jn));
        m_pObj2->SetAngularVelocity(m_pObj2->GetAngularVelocity() - m_pObj2->GetInverseInertia() * Maths::Vector3::Cross(r2, abn * jn));
    }

    void SpringConstraint::DebugDraw() const
    {
        Maths::Vector3 globalOnA = m_pObj1->GetOrientation().RotationMatrix() * m_LocalOnA + m_pObj1->GetPosition();
        Maths::Vector3 globalOnB = m_pObj2->GetOrientation().RotationMatrix() * m_LocalOnB + m_pObj2->GetPosition();

        DebugRenderer::DrawThickLine(globalOnA, globalOnB, 0.02f, Maths::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
        DebugRenderer::DrawPointNDT(globalOnA, 0.05f, Maths::Vector4(1.0f, 0.8f, 1.0f, 1.0f));
        DebugRenderer::DrawPointNDT(globalOnB, 0.05f, Maths::Vector4(1.0f, 0.8f, 1.0f, 1.0f));
    }
}
