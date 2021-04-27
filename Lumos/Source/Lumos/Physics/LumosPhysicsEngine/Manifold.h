#pragma once

#include "RigidBody3D.h"
#include "Maths/Maths.h"

namespace Lumos
{
    /* A contact constraint is actually the summation of a normal distance constraint
	along with two friction constraints going along the axes perpendicular to the collision
	normal.
	*/
    struct LUMOS_EXPORT ContactPoint
    {
        float sumImpulseContact = 0.0f;
        float sumImpulseFriction = 0.0f;
        float elatisity_term = 0.0f;
        float collisionPenetration = 0.0f;

        Maths::Vector3 collisionNormal;
        Maths::Vector3 relPosA; //Position relative to objectA
        Maths::Vector3 relPosB; //Position relative to objectB
    };
#define MAX_CONTACT_POINTS 8

    class LUMOS_EXPORT Manifold
    {
    public:
        Manifold();
        ~Manifold();

        //Initiate for collision pair
        void Initiate(RigidBody3D* nodeA, RigidBody3D* nodeB);

        //Called whenever a new collision contact between A & B are found
        void AddContact(const Maths::Vector3& globalOnA, const Maths::Vector3& globalOnB, const Maths::Vector3& _normal, const float& _penetration);

        //Sequentially solves each contact constraint
        void ApplyImpulse();
        void PreSolverStep(float dt);

        //Debug draws the manifold surface area
        void DebugDraw() const;

        //Get the physics objects
        RigidBody3D* NodeA() const
        {
            return m_pNodeA;
        }
        RigidBody3D* NodeB() const
        {
            return m_pNodeB;
        }

    protected:
        void SolveContactPoint(ContactPoint& c) const;
        void UpdateConstraint(ContactPoint& c);

    protected:
        RigidBody3D* m_pNodeA;
        RigidBody3D* m_pNodeB;
        ContactPoint m_vContacts[MAX_CONTACT_POINTS];
        uint32_t m_ContactCount = 0;
    };
}
