#pragma once

#include "Maths/Vector3.h"

namespace Lumos
{
    class RigidBody3D;
    /* A contact constraint is actually the summation of a normal distance constraint
        along with two friction constraints going along the axes perpendicular to the collision
        normal.
        */
    struct LUMOS_EXPORT ContactPoint
    {
        float sumImpulseContact    = 0.0f;
        float sumImpulseFriction1  = 0.0f;
        float sumImpulseFriction2  = 0.0f;
        // Rolling friction angular impulses, along frictionTangent1/2.
        // Decouples linear slip resistance (sumImpulseFriction*) from rolling
        // resistance (sumImpulseRolling*); rolling axes are the same tangent plane.
        float sumImpulseRolling1   = 0.0f;
        float sumImpulseRolling2   = 0.0f;
        float elatisity_term       = 0.0f;
        float collisionPenetration = 0.0f;

        Vec3 collisionNormal;
        Vec3 frictionTangent1;
        Vec3 frictionTangent2;
        Vec3 relPosA; // Position relative to objectA
        Vec3 relPosB; // Position relative to objectB
    };
#define MAX_CONTACT_POINTS 16

    class LUMOS_EXPORT Manifold
    {
    public:
        Manifold();
        ~Manifold();

        // Initiate for collision pair
        void Initiate(RigidBody3D* nodeA, RigidBody3D* nodeB, float BaumgarteScalar, float BaumgarteSlop);

        // Called whenever a new collision contact between A & B are found
        void AddContact(const Vec3& globalOnA, const Vec3& globalOnB, const Vec3& _normal, const float& _penetration);

        // Sequentially solves each contact constraint
        void ApplyImpulse();
        void PreSolverStep(float dt);

        // Debug draws the manifold surface area
        void DebugDraw() const;

        // Get the physics objects
        RigidBody3D* NodeA() const
        {
            return m_pNodeA;
        }
        RigidBody3D* NodeB() const
        {
            return m_pNodeB;
        }

        // Contact accessors (used by warm-starting)
        ContactPoint* GetContacts() { return m_vContacts; }
        const ContactPoint* GetContacts() const { return m_vContacts; }
        uint32_t GetContactCount() const { return m_ContactCount; }

    protected:
        void SolveContactPoint(ContactPoint& c) const;
        void UpdateConstraint(ContactPoint& c);

    protected:
        RigidBody3D* m_pNodeA;
        RigidBody3D* m_pNodeB;
        ContactPoint m_vContacts[MAX_CONTACT_POINTS];
        uint32_t m_ContactCount = 0;
        float m_BaumgarteScalar = 0.2f;   // Amount of force to add to the System to solve error
        float m_BaumgarteSlop   = 0.001f; // Amount of allowed penetration, ensures a complete manifold each frame
    };
}
