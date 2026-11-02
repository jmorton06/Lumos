#include "Precompiled.h"
#include "Manifold.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/RigidBody3D.h"
#include "Physics/LumosPhysicsEngine/PhysicsMaterial.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"

#include "Maths/Matrix3.h"

namespace Lumos
{
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

        if(c.normalMass < Maths::M_EPSILON)
            return;

        const bool dynA = m_pNodeA->GetInverseMass() > 0.0f;
        const bool dynB = m_pNodeB->GetInverseMass() > 0.0f;

        Vec3& r1 = c.relPosA;
        Vec3& r2 = c.relPosB;

        Vec3& normal = c.collisionNormal;

        // ---- Normal (contact) impulse ----
        {
            Vec3 v0 = m_pNodeA->GetLinearVelocity() + Maths::Cross(m_pNodeA->GetAngularVelocity(), r1);
            Vec3 v1 = m_pNodeB->GetLinearVelocity() + Maths::Cross(m_pNodeB->GetAngularVelocity(), r2);
            Vec3 dv = v0 - v1;

            float jn = -(Maths::Dot(dv, normal) + c.normalBias) * c.normalMass;

            // Clamp accumulated impulse (not the per-iter delta) — allows iterations to refine
            float oldSum        = c.sumImpulseContact;
            c.sumImpulseContact = Maths::Min(oldSum + jn, 0.0f);
            jn                  = c.sumImpulseContact - oldSum;

            if(dynA)
            {
                m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                                            + normal * (jn * m_pNodeA->GetInverseMass()) * m_pNodeA->GetLinearFactor());
                m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                                             + m_pNodeA->GetInverseInertia() * Maths::Cross(r1, normal * jn) * m_pNodeA->GetAngularFactor());
            }
            if(dynB)
            {
                m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                                            - normal * (jn * m_pNodeB->GetInverseMass()) * m_pNodeB->GetLinearFactor());
                m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                                             - m_pNodeB->GetInverseInertia() * Maths::Cross(r2, normal * jn) * m_pNodeB->GetAngularFactor());
            }
        }

        // ---- 2D friction (Box2D-style, fixed tangent basis) ----
        {
            // Recompute dv after normal impulse was applied
            Vec3 v0 = m_pNodeA->GetLinearVelocity() + Maths::Cross(m_pNodeA->GetAngularVelocity(), r1);
            Vec3 v1 = m_pNodeB->GetLinearVelocity() + Maths::Cross(m_pNodeB->GetAngularVelocity(), r2);
            Vec3 dv = v0 - v1;

            const float frictionCoef = c.frictionCoef;

            const Vec3& t1 = c.frictionTangent1;
            const Vec3& t2 = c.frictionTangent2;

            if(c.tangentMass1 < Maths::M_EPSILON || c.tangentMass2 < Maths::M_EPSILON)
                return;

            float jt1 = -Maths::Dot(dv, t1) * c.tangentMass1;
            float jt2 = -Maths::Dot(dv, t2) * c.tangentMass2;

            float oldSum1 = c.sumImpulseFriction1;
            float oldSum2 = c.sumImpulseFriction2;

            float newSum1 = oldSum1 + jt1;
            float newSum2 = oldSum2 + jt2;

            float maxFriction = -frictionCoef * c.sumImpulseContact;
            float mag2        = newSum1 * newSum1 + newSum2 * newSum2;
            if(mag2 > maxFriction * maxFriction && mag2 > Maths::M_EPSILON)
            {
                float scale = maxFriction / Maths::Sqrt(mag2);
                newSum1 *= scale;
                newSum2 *= scale;
            }

            c.sumImpulseFriction1 = newSum1;
            c.sumImpulseFriction2 = newSum2;

            float delta1 = newSum1 - oldSum1;
            float delta2 = newSum2 - oldSum2;

            Vec3 impulseA = t1 * delta1 + t2 * delta2;

            if(dynA)
            {
                m_pNodeA->SetLinearVelocity(m_pNodeA->GetLinearVelocity()
                                            + impulseA * m_pNodeA->GetInverseMass() * m_pNodeA->GetLinearFactor());
                m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                                             + m_pNodeA->GetInverseInertia() * Maths::Cross(r1, impulseA) * m_pNodeA->GetAngularFactor());
            }
            if(dynB)
            {
                m_pNodeB->SetLinearVelocity(m_pNodeB->GetLinearVelocity()
                                            - impulseA * m_pNodeB->GetInverseMass() * m_pNodeB->GetLinearFactor());
                m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                                             - m_pNodeB->GetInverseInertia() * Maths::Cross(r2, impulseA) * m_pNodeB->GetAngularFactor());
            }
        }

        {
            const float rollCoef = c.rollingCoef;

            if(rollCoef > 0.0f)
            {
                Vec3 relAng = m_pNodeA->GetAngularVelocity() - m_pNodeB->GetAngularVelocity();

                const Vec3& t1 = c.frictionTangent1;
                const Vec3& t2 = c.frictionTangent2;

                if(c.rollingMass1 > Maths::M_EPSILON && c.rollingMass2 > Maths::M_EPSILON)
                {
                    // Unconstrained angular impulse to zero the rolling component.
                    float jr1 = -Maths::Dot(relAng, t1) * c.rollingMass1;
                    float jr2 = -Maths::Dot(relAng, t2) * c.rollingMass2;

                    float oldR1 = c.sumImpulseRolling1;
                    float oldR2 = c.sumImpulseRolling2;
                    float newR1 = oldR1 + jr1;
                    float newR2 = oldR2 + jr2;

                    // Dimensionally: rollCoef [m] * |normalImpulse [kg m/s]| = [kg m^2/s] (angular impulse).
                    float maxRoll = rollCoef * Maths::Abs(c.sumImpulseContact);
                    float mag2    = newR1 * newR1 + newR2 * newR2;
                    if(mag2 > maxRoll * maxRoll && mag2 > Maths::M_EPSILON)
                    {
                        float scale = maxRoll / Maths::Sqrt(mag2);
                        newR1 *= scale;
                        newR2 *= scale;
                    }

                    c.sumImpulseRolling1 = newR1;
                    c.sumImpulseRolling2 = newR2;

                    float dR1 = newR1 - oldR1;
                    float dR2 = newR2 - oldR2;

                    Vec3 angularImpulse = t1 * dR1 + t2 * dR2;

                    if(dynA)
                        m_pNodeA->SetAngularVelocity(m_pNodeA->GetAngularVelocity()
                                                     + m_pNodeA->GetInverseInertia() * angularImpulse * m_pNodeA->GetAngularFactor());
                    if(dynB)
                        m_pNodeB->SetAngularVelocity(m_pNodeB->GetAngularVelocity()
                                                     - m_pNodeB->GetInverseInertia() * angularImpulse * m_pNodeB->GetAngularFactor());
                }
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
        contact.sumImpulseContact   = 0.0f;
        contact.sumImpulseFriction1 = 0.0f;
        contact.sumImpulseFriction2 = 0.0f;
        contact.sumImpulseRolling1  = 0.0f;
        contact.sumImpulseRolling2  = 0.0f;

        const Vec3& n = contact.collisionNormal;
        Vec3 ref      = (Maths::Abs(n.x) > 0.57735f) ? Vec3(0.0f, 1.0f, 0.0f) : Vec3(1.0f, 0.0f, 0.0f);
        Vec3 t1       = Maths::Cross(ref, n);
        float len     = Maths::Length(t1);
        if(len > Maths::M_EPSILON)
            t1 = t1 * (1.0f / len);
        else
            t1 = Vec3(1.0f, 0.0f, 0.0f);
        contact.frictionTangent1 = t1;
        contact.frictionTangent2 = Maths::Cross(n, t1);

        // Compute Elasticity Term - must be computed prior to solving
        // ANY constraints otherwise the objects velocities may have
        // already changed in a different constraint and the elasticity
        // force will no longer be correct .
        {
            const auto& matA = m_pNodeA->GetMaterial();
            const auto& matB = m_pNodeB->GetMaterial();
            const float elasticity = PhysicsMaterial::CombineValues(matA.Restitution, matB.Restitution, matA.RestitutionCombine);

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

        {
            const Vec3& r1   = contact.relPosA;
            const Vec3& r2   = contact.relPosB;
            const Vec3& t1   = contact.frictionTangent1;
            const Vec3& t2   = contact.frictionTangent2;
            const float invM = m_pNodeA->GetInverseMass() + m_pNodeB->GetInverseMass();
            const Mat3& invIA = m_pNodeA->GetInverseInertia();
            const Mat3& invIB = m_pNodeB->GetInverseInertia();

            auto effMass = [&](const Vec3& axis) -> float
            {
                return invM + Maths::Dot(axis,
                                         Maths::Cross(invIA * Maths::Cross(r1, axis), r1)
                                             + Maths::Cross(invIB * Maths::Cross(r2, axis), r2));
            };

            float kn  = effMass(n);
            float kt1 = effMass(t1);
            float kt2 = effMass(t2);
            contact.normalMass   = (kn > Maths::M_EPSILON) ? 1.0f / kn : 0.0f;
            contact.tangentMass1 = (kt1 > Maths::M_EPSILON) ? 1.0f / kt1 : 0.0f;
            contact.tangentMass2 = (kt2 > Maths::M_EPSILON) ? 1.0f / kt2 : 0.0f;

            // Rolling friction effective masses (pure rotational inertia about each tangent).
            float kr1 = Maths::Dot(t1, invIA * t1) + Maths::Dot(t1, invIB * t1);
            float kr2 = Maths::Dot(t2, invIA * t2) + Maths::Dot(t2, invIB * t2);
            contact.rollingMass1 = (kr1 > Maths::M_EPSILON) ? 1.0f / kr1 : 0.0f;
            contact.rollingMass2 = (kr2 > Maths::M_EPSILON) ? 1.0f / kr2 : 0.0f;

            // Normal bias (Baumgarte + restitution) — constant per timestep.
            float penetrationSlop = Maths::Min(contact.collisionPenetration + m_BaumgarteSlop, 0.0f);
            float b               = -(m_BaumgarteScalar / LumosPhysicsEngine::GetDeltaTime()) * penetrationSlop;
            contact.normalBias    = Maths::Max(b, contact.elatisity_term + b * 0.2f);

            // Material combines — also constant per timestep.
            const auto& matA      = m_pNodeA->GetMaterial();
            const auto& matB      = m_pNodeB->GetMaterial();
            contact.frictionCoef  = PhysicsMaterial::CombineValues(matA.Friction, matB.Friction, matA.FrictionCombine);
            contact.rollingCoef   = PhysicsMaterial::CombineValues(matA.RollingFriction, matB.RollingFriction, matA.RollingFrictionCombine);
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
        contact.elatisity_term        = 1.0f;
        contact.sumImpulseContact     = 0.0f;
        contact.sumImpulseFriction1   = 0.0f;
        contact.sumImpulseFriction2   = 0.0f;
        contact.sumImpulseRolling1    = 0.0f;
        contact.sumImpulseRolling2    = 0.0f;

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

        if(should_add && m_ContactCount < MAX_CONTACT_POINTS)
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
                DebugRenderer::DrawThickLine(globalOnA1, globalOnA2, DEBUG_LINE_WIDTH, false, Vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.0f);

                // Draw descriptors for indivdual contact point
                DebugRenderer::DrawPoint(globalOnA2, 0.05f, false, Vec4(0.0f, 0.5f, 0.0f, 1.0f), 0.0f);
                DebugRenderer::DrawThickLine(globalOnB, globalOnA2, DEBUG_LINE_WIDTH * 0.75f, false, Vec4(1.0f, 0.0f, 1.0f, 1.0f), 0.0f);

                globalOnA1 = globalOnA2;
            }
        }
    }
}
