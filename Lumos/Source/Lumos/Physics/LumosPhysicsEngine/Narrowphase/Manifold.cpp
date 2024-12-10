#include "Precompiled.h"
#include "Manifold.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"

#include "Maths/Matrix3.h"

namespace Lumos
{

#define persistentThresholdSq 0.025f

    Manifold::Manifold()
        : m_pNodeA(nullptr)
        , m_pNodeB(nullptr)
    {
    }

    Manifold::~Manifold()
    {
    }

    void Manifold::Initiate(RigidBody3D* nodeA, RigidBody3D* nodeB, float BaumgarteScalar, float BaumgarteSlop)
    {
        m_ContactCount = 0;

        m_pNodeA = nodeA;
        m_pNodeB = nodeB;

        m_BaumgarteScalar = BaumgarteScalar;
        m_BaumgarteSlop   = BaumgarteSlop;
    }

    void Manifold::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            SolveContactPoint(m_vContacts[i]);
        }
    }

    void Manifold::SolveContactPoint(ContactPoint& c) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        if(m_pNodeA->GetInverseMass() + m_pNodeB->GetInverseMass() < Maths::M_EPSILON)
            return;

        Vec3& r1 = c.relPosA;
        Vec3& r2 = c.relPosB;

        Vec3 v0 = m_pNodeA->GetLinearVelocity() + Maths::Cross(m_pNodeA->GetAngularVelocity(), r1);
        Vec3 v1 = m_pNodeB->GetLinearVelocity() + Maths::Cross(m_pNodeB->GetAngularVelocity(), r2);

        Vec3& normal = c.collisionNormal;
        Vec3 dv      = v0 - v1;

        // Collision Resoluton
        {
            const float constraintMass = (m_pNodeA->GetInverseMass()
                                          + m_pNodeB->GetInverseMass())
                + Maths::Dot(normal,
                             Maths::Cross(m_pNodeA->GetInverseInertia()
                                              * Maths::Cross(r1, normal),
                                          r1)
                                 + Maths::Cross(m_pNodeB->GetInverseInertia()
                                                    * Maths::Cross(r2, normal),
                                                r2));
            // Baumgarte Offset ( Adds energy to the System to counter
            // slight solving errors that accumulate over time
            // called as �constraint drift �)

            float penetrationSlop      = Maths::Min(c.collisionPenetration + m_BaumgarteSlop, 0.0f);
            float b                    = -(m_BaumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * penetrationSlop;
            float b_real               = Maths::Max(b, c.elatisity_term + b * 0.2f);
            float jn                   = -(Maths::Dot(dv, normal) + b_real) / constraintMass;
            float oldSumImpulseContact = c.sumImpulseContact;

            jn                  = Maths::Min(jn, 0.0f);
            c.sumImpulseContact = Maths::Min(c.sumImpulseContact + jn, 0.0f);
            jn                  = c.sumImpulseContact - oldSumImpulseContact;

            m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                                        + normal * (jn * m_pNodeA->GetInverseMass()));
            m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                                        - normal * (jn * m_pNodeB->GetInverseMass()));

            m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                                         + m_pNodeA->GetInverseInertia()
                                             * Maths::Cross(r1, normal * jn));
            m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                                         - m_pNodeB->GetInverseInertia()
                                             * Maths::Cross(r2, normal * jn));
        }
        // Friction
        {
            Vec3 tangent      = dv - normal * Maths::Dot(dv, normal);
            float tangent_len = Maths::Length(tangent);

            if(tangent_len > Maths::M_EPSILON)
            {
                tangent = tangent * (1.0f / tangent_len);

                float frictionalMass = (m_pNodeA->GetInverseMass() + m_pNodeB->GetInverseMass())
                    + Maths::Dot(tangent, Maths::Cross(m_pNodeA->GetInverseInertia() * Maths::Cross(r1, tangent), r1) + Maths::Cross(m_pNodeB->GetInverseInertia() * Maths::Cross(r2, tangent), r2));
                float frictionCoef = Maths::Sqrt(m_pNodeA->GetFriction() * m_pNodeB->GetFriction());
                float jt           = -1.0f * frictionCoef * Maths::Dot(dv, tangent) / frictionalMass;

                // Clamp friction to never apply more force than the main collision
                // resolution force

                float oldImpulseTangent = c.sumImpulseFriction;
                float maxJt             = frictionCoef * c.sumImpulseContact;
                c.sumImpulseFriction    = Maths::Min(Maths::Max(oldImpulseTangent + jt, maxJt), -maxJt);
                jt                      = c.sumImpulseFriction - oldImpulseTangent;

                m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                                            + tangent * (jt * m_pNodeA->GetInverseMass()));
                m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                                            - tangent * (jt * m_pNodeB->GetInverseMass()));

                m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                                             + m_pNodeA->GetInverseInertia()
                                                 * Maths::Cross(r1, tangent * jt));
                m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                                             - m_pNodeB->GetInverseInertia()
                                                 * Maths::Cross(r2, tangent * jt));
            }
        }
    }

    void Manifold::PreSolverStep(float dt)
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            UpdateConstraint(m_vContacts[i]);
        }
    }

    void Manifold::UpdateConstraint(ContactPoint& contact)
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        // Reset total impulse forces computed this physics timestep
        contact.sumImpulseContact  = 0.0f;
        contact.sumImpulseFriction = 0.0f;

        // Compute Elasticity Term - must be computed prior to solving
        // ANY constraints otherwise the objects velocities may have
        // already changed in a different constraint and the elasticity
        // force will no longer be correct .
        {
            const float elasticity = sqrtf(m_pNodeA->GetElasticity()
                                           * m_pNodeB->GetElasticity());

            float elatisity_term = elasticity * Maths::Dot(contact.collisionNormal, m_pNodeA->GetLinearVelocity() + Maths::Cross(contact.relPosA, m_pNodeA->GetAngularVelocity()) - m_pNodeB->GetLinearVelocity() - Maths::Cross(contact.relPosB, m_pNodeB->GetAngularVelocity()));

            if(elatisity_term < 0.0f)
            {
                contact.elatisity_term = 0.0f;
            }
            else
            {
                // Elasticity slop here is used to make objects come to
                // rest quicker . It works out if the elastic term is less
                // than a given value (0.2 m/s here ) and if it is , then we
                // assume it is too small to see and should ignore the
                // elasticity calculation . Most noticeable when you have a
                // stack of objects , without this they will jitter a lot .

                const float elasticity_slop = 0.2f;

                if(elatisity_term < elasticity_slop)
                    elatisity_term = 0.0f;

                contact.elatisity_term = elatisity_term;
            }
        }
    }

    void Manifold::AddContact(const Vec3& globalOnA, const Vec3& globalOnB, const Vec3& _normal, const float& _penetration)
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        // Get relative offsets from each object centre of mass
        //  Used to compute rotational velocity at the point of contact.
        Vec3 r1 = (globalOnA - m_pNodeA->GetPosition());
        Vec3 r2 = (globalOnB - m_pNodeB->GetPosition());

        // Create our new contact descriptor
        ContactPoint contact;
        contact.relPosA              = r1;
        contact.relPosB              = r2;
        contact.collisionNormal      = _normal;
        contact.collisionPenetration = _penetration;
        contact.elatisity_term       = 1.0f;
        contact.sumImpulseContact    = 0.0f;
        contact.sumImpulseFriction   = 0.0f;

        // Check to see if we already contain a contact point almost in that location
        const float min_allowed_dist_sq = 0.2f * 0.2f;
        bool should_add                 = true;
        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            Vec3 ab      = m_vContacts[i].relPosA - contact.relPosA;
            float distsq = Maths::Dot(ab, ab);

            // Choose the contact point with the largest penetration and therefore the largest collision response
            if(distsq < min_allowed_dist_sq)
            {
                if(m_vContacts[i].collisionPenetration > contact.collisionPenetration)
                {
                    Swap(m_vContacts[i], m_vContacts[m_ContactCount - 1]);
                    m_ContactCount--;
                    i--;
                    continue;
                }
                else
                {
                    should_add = false;
                }
            }
        }

        if(should_add)
        {
            m_vContacts[m_ContactCount] = contact;
            m_ContactCount++;
        }
    }

    void Manifold::DebugDraw() const
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        if(m_ContactCount > 0)
        {
            // Loop around all contact points and draw them all as a line-fan
            Vec3 globalOnA1 = m_pNodeA->GetPosition() + m_vContacts[m_ContactCount - 1].relPosA;
            for(uint32_t i = 0; i < m_ContactCount; i++)
            {
                auto& contact   = m_vContacts[i];
                Vec3 globalOnA2 = m_pNodeA->GetPosition() + contact.relPosA;
                Vec3 globalOnB  = m_pNodeB->GetPosition() + contact.relPosB;

                // Draw line to form area given by all contact points
                DebugRenderer::DrawThickLine(globalOnA1, globalOnA2, 0.02f, false, Vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.0f);

                // Draw descriptors for indivdual contact point
                DebugRenderer::DrawPoint(globalOnA2, 0.05f, false, Vec4(0.0f, 0.5f, 0.0f, 1.0f), 0.0f);
                DebugRenderer::DrawThickLine(globalOnB, globalOnA2, 0.01f, false, Vec4(1.0f, 0.0f, 1.0f, 1.0f), 0.0f);

                globalOnA1 = globalOnA2;
            }
        }
    }
}
