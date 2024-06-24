#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "DistanceConstraint.h"
#include "Graphics/Renderers/DebugRenderer.h"

namespace Lumos
{

    DistanceConstraint::DistanceConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const glm::vec3& globalOnA, const glm::vec3& globalOnB)
        : m_pObj1(obj1)
        , m_pObj2(obj2)
    {
        glm::vec3 ab = globalOnB - globalOnA;
        m_Distance   = glm::length(ab);

        glm::vec3 r1 = globalOnA - m_pObj1->GetPosition();
        glm::vec3 r2 = globalOnB - m_pObj2->GetPosition();
        m_LocalOnA   = glm::toMat3(m_pObj1->GetOrientation()) * r1;
        m_LocalOnB   = glm::toMat3(m_pObj1->GetOrientation()) * r2;
    }

    void DistanceConstraint::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass() == 0.0f)
            return;

        glm::vec3 r1 = glm::toMat3(m_pObj1->GetOrientation()) * m_LocalOnA;
        glm::vec3 r2 = glm::toMat3(m_pObj2->GetOrientation()) * m_LocalOnB;

        glm::vec3 globalOnA = r1 + m_pObj1->GetPosition();
        glm::vec3 globalOnB = r2 + m_pObj2->GetPosition();

        glm::vec3 ab  = globalOnB - globalOnA;
        glm::vec3 abn = ab;
        abn           = glm::normalize(abn);

        glm::vec3 v0         = m_pObj1->GetLinearVelocity() + glm::cross(m_pObj1->GetAngularVelocity(), r1);
        glm::vec3 v1         = m_pObj2->GetLinearVelocity() + glm::cross(m_pObj2->GetAngularVelocity(), r2);
        float constraintMass = (m_pObj1->GetInverseMass() + m_pObj2->GetInverseMass()) + glm::dot(abn, glm::cross(m_pObj1->GetInverseInertia() * glm::cross(r1, abn), r1) + glm::cross(m_pObj2->GetInverseInertia() * glm::cross(r2, abn), r2));

        float b = 0.0f;
        {
            float distanceOffset  = glm::length(ab) - m_Distance;
            float baumgarteScalar = 0.1f;
            b                     = -(baumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * distanceOffset;
        }

        float jn = -(glm::dot(v0 - v1, abn) + b) / constraintMass;

        m_pObj1->SetLinearVelocity(m_pObj1->GetLinearVelocity() + abn * (jn * m_pObj1->GetInverseMass()));
        m_pObj2->SetLinearVelocity(m_pObj2->GetLinearVelocity() - abn * (jn * m_pObj2->GetInverseMass()));

        m_pObj1->SetAngularVelocity(m_pObj1->GetAngularVelocity() + m_pObj1->GetInverseInertia() * glm::cross(r1, abn * jn));
        m_pObj2->SetAngularVelocity(m_pObj2->GetAngularVelocity() - m_pObj2->GetInverseInertia() * glm::cross(r2, abn * jn));
    }

    void DistanceConstraint::DebugDraw() const
    {
        glm::vec3 globalOnA = glm::toMat3(m_pObj1->GetOrientation()) * m_LocalOnA + m_pObj1->GetPosition();
        glm::vec3 globalOnB = glm::toMat3(m_pObj2->GetOrientation()) * m_LocalOnB + m_pObj2->GetPosition();

        DebugRenderer::DrawThickLine(globalOnA, globalOnB, 0.02f, false, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        DebugRenderer::DrawPoint(globalOnA, 0.05f, false, glm::vec4(1.0f, 0.8f, 1.0f, 1.0f));
        DebugRenderer::DrawPoint(globalOnB, 0.05f, false, glm::vec4(1.0f, 0.8f, 1.0f, 1.0f));
    }
}
