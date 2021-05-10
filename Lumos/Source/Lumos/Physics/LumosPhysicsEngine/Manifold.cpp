#include "Precompiled.h"
#include "Manifold.h"
#include "Maths/Matrix3.h"

#include "LumosPhysicsEngine.h"

#include "Graphics/Renderers/DebugRenderer.h"

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

    void Manifold::Initiate(RigidBody3D* nodeA, RigidBody3D* nodeB)
    {
        m_ContactCount = 0;

        m_pNodeA = nodeA;
        m_pNodeB = nodeB;
    }

    void Manifold::ApplyImpulse()
    {
        LUMOS_PROFILE_FUNCTION();
        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            SolveContactPoint(m_vContacts[i]);
        }
    }

    void Manifold::SolveContactPoint(ContactPoint& c) const
    {
        LUMOS_PROFILE_FUNCTION();

        if(m_pNodeA->GetInverseMass() + m_pNodeB->GetInverseMass() == 0.0f)
            return;

        Maths::Vector3 r1 = c.relPosA;
        Maths::Vector3 r2 = c.relPosB;

        Maths::Vector3 v0 = m_pNodeA->GetLinearVelocity() + Maths::Vector3::Cross(m_pNodeA->GetAngularVelocity(), r1);
        Maths::Vector3 v1 = m_pNodeB->GetLinearVelocity() + Maths::Vector3::Cross(m_pNodeB->GetAngularVelocity(), r2);

        Maths::Vector3 normal = c.collisionNormal;
        Maths::Vector3 dv = v0 - v1;

        // Collision Resolution
        {
            const float constraintMass = (m_pNodeA->GetInverseMass()
                                             + m_pNodeB->GetInverseMass())
                + Maths::Vector3::Dot(normal,
                    Maths::Vector3::Cross(m_pNodeA->GetInverseInertia()
                            * Maths::Vector3::Cross(r1, normal),
                        r1)
                        + Maths::Vector3::Cross(m_pNodeB->GetInverseInertia()
                                * Maths::Vector3::Cross(r2, normal),
                            r2));
            // Baumgarte Offset ( Adds energy to the System to counter
            // slight solving errors that accumulate over time
            // called as �constraint drift �)

            float b;
            {
                //float distanceOffset = c.collisionPenetration;

                float baumgarteScalar = 0.3f; // Amount of force to add to the System to solve error
                float baumgarteSlop = 0.001f; // Amount of allowed penetration, ensures a complete manifold each frame

                float penetrationSlop = Maths::Min(c.collisionPenetration + baumgarteSlop, 0.0f);

                b = -(baumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * penetrationSlop;
            }

            float b_real = Maths::Max(b, c.elatisity_term + b * 0.2f);
            float jn = -(Maths::Vector3::Dot(dv, normal) + b_real) / constraintMass;

            //jn = min(jn, 0.0f);
            float oldSumImpulseContact = c.sumImpulseContact;
            c.sumImpulseContact = Maths::Min(c.sumImpulseContact + jn, 0.0f);
            jn = c.sumImpulseContact - oldSumImpulseContact;

            m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                + normal * (jn * m_pNodeA->GetInverseMass()));
            m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                - normal * (jn * m_pNodeB->GetInverseMass()));

            m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                + m_pNodeA->GetInverseInertia()
                    * Maths::Vector3::Cross(r1, normal * jn));
            m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                - m_pNodeB->GetInverseInertia()
                    * Maths::Vector3::Cross(r2, normal * jn));
        }
        // Friction
        {
            Maths::Vector3 tangent = dv - normal * Maths::Vector3::Dot(dv, normal);
            float tangent_len = tangent.Length();

            if(tangent_len > 0.001f)
            {
                tangent = tangent * (1.0f / tangent_len);

                float frictionalMass = (m_pNodeA->GetInverseMass()
                                           + m_pNodeB->GetInverseMass())
                    + Maths::Vector3::Dot(tangent,
                        Maths::Vector3::Cross(m_pNodeA->GetInverseInertia()
                                * Maths::Vector3::Cross(r1, tangent),
                            r1)
                            + Maths::Vector3::Cross(m_pNodeB->GetInverseInertia()
                                    * Maths::Vector3::Cross(r2, tangent),
                                r2));

                float frictionCoef = sqrtf(m_pNodeA->GetFriction()
                    * m_pNodeB->GetFriction());

                float jt = -1 * frictionCoef * Maths::Vector3::Dot(dv, tangent)
                    / frictionalMass;

                // Clamp friction to never apply more force than the main collision
                // resolution force

                float oldImpulseTangent = c.sumImpulseFriction;
                float maxJt = frictionCoef * c.sumImpulseContact;

                c.sumImpulseFriction = Maths::Min(Maths::Max(oldImpulseTangent + jt, maxJt), -maxJt);

                jt = c.sumImpulseFriction - oldImpulseTangent;

                m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                    + tangent * (jt * m_pNodeA->GetInverseMass()));
                m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                    - tangent * (jt * m_pNodeB->GetInverseMass()));

                m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                    + m_pNodeA->GetInverseInertia()
                        * Maths::Vector3::Cross(r1, tangent * jt));
                m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                    - m_pNodeB->GetInverseInertia()
                        * Maths::Vector3::Cross(r2, tangent * jt));
            }
        }
    }

    void Manifold::PreSolverStep(float dt)
    {
        LUMOS_PROFILE_FUNCTION();

        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            UpdateConstraint(m_vContacts[i]);
        }
    }

    void Manifold::UpdateConstraint(ContactPoint& contact)
    {
        LUMOS_PROFILE_FUNCTION();

        //Reset total impulse forces computed this physics timestep
        contact.sumImpulseContact = 0.0f;
        contact.sumImpulseFriction = 0.0f;

        // Compute Elasticity Term - must be computed prior to solving
        // ANY constraints otherwise the objects velocities may have
        // already changed in a different constraint and the elasticity
        // force will no longer be correct .
        {
            const float elasticity = sqrtf(m_pNodeA->GetElasticity()
                * m_pNodeB->GetElasticity());

            float elatisity_term = elasticity * Maths::Vector3::Dot(contact.collisionNormal, m_pNodeA->GetLinearVelocity() + Maths::Vector3::Cross(contact.relPosA, m_pNodeA->GetAngularVelocity()) - m_pNodeB->GetLinearVelocity() - Maths::Vector3::Cross(contact.relPosB, m_pNodeB->GetAngularVelocity()));

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
                // elasticity calculation . Most noticable when you have a
                // stack of objects , without this they will jitter alot .

                const float elasticity_slop = 0.2f;

                if(elatisity_term < elasticity_slop)
                    elatisity_term = 0.0f;

                contact.elatisity_term = elatisity_term;
            }
        }
    }

    void Manifold::AddContact(const Maths::Vector3& globalOnA, const Maths::Vector3& globalOnB, const Maths::Vector3& _normal, const float& _penetration)
    {
        LUMOS_PROFILE_FUNCTION();
        //Get relative offsets from each object centre of mass
        // Used to compute rotational velocity at the point of contact.
        Maths::Vector3 r1 = (globalOnA - m_pNodeA->GetPosition());
        Maths::Vector3 r2 = (globalOnB - m_pNodeB->GetPosition());

        //Create our new contact descriptor
        ContactPoint contact;
        contact.relPosA = r1;
        contact.relPosB = r2;
        contact.collisionNormal = _normal;
        contact.collisionPenetration = _penetration;
        contact.elatisity_term = 1.0f;
        contact.sumImpulseContact = 0.0f;
        contact.sumImpulseFriction = 0.0f;

        //Check to see if we already contain a contact point almost in that location
        const float min_allowed_dist_sq = 0.2f * 0.2f;
        bool should_add = true;
        for(uint32_t i = 0; i < m_ContactCount; i++)
        {
            Maths::Vector3 ab = m_vContacts[i].relPosA - contact.relPosA;
            float distsq = Maths::Vector3::Dot(ab, ab);

            //Choose the contact point with the largest penetration and therefore the largest collision response
            if(distsq < min_allowed_dist_sq)
            {
                if(m_vContacts[i].collisionPenetration > contact.collisionPenetration)
                {
                    std::swap(m_vContacts[i], m_vContacts[m_ContactCount - 1]);
                    m_ContactCount--;
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
        LUMOS_PROFILE_FUNCTION();

        if(m_ContactCount > 0)
        {
            //Loop around all contact points and draw them all as a line-fan
            Maths::Vector3 globalOnA1 = m_pNodeA->GetPosition() + m_vContacts[m_ContactCount - 1].relPosA;
            for(uint32_t i = 0; i < m_ContactCount; i++)
            {
                auto& contact = m_vContacts[i];
                Maths::Vector3 globalOnA2 = m_pNodeA->GetPosition() + contact.relPosA;
                Maths::Vector3 globalOnB = m_pNodeB->GetPosition() + contact.relPosB;

                //Draw line to form area given by all contact points
                DebugRenderer::DrawThickLineNDT(globalOnA1, globalOnA2, 0.02f, Maths::Vector4(0.0f, 1.0f, 0.0f, 1.0f));

                //Draw descriptors for indivdual contact point
                DebugRenderer::DrawPointNDT(globalOnA2, 0.05f, Maths::Vector4(0.0f, 0.5f, 0.0f, 1.0f));
                DebugRenderer::DrawThickLineNDT(globalOnB, globalOnA2, 0.01f, Maths::Vector4(1.0f, 0.0f, 1.0f, 1.0f));

                globalOnA1 = globalOnA2;
            }
        }
    }
}
