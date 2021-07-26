#pragma once

#include "Utilities/TSingleton.h"
#include "RigidBody3D.h"
#include "Manifold.h"
#include "Broadphase.h"
#include "Scene/ISystem.h"
#include "Scene/Scene.h"

namespace Lumos
{

#define SOLVER_ITERATIONS 20

    enum class LUMOS_EXPORT IntegrationType
    {
        EXPLICIT_EULER = 0,
        SEMI_IMPLICIT_EULER,
        RUNGE_KUTTA_2,
        RUNGE_KUTTA_4
    };

    enum PhysicsDebugFlags : uint32_t
    {
        CONSTRAINT = 1,
        MANIFOLD = 2,
        COLLISIONVOLUMES = 4,
        COLLISIONNORMALS = 8,
        AABB = 16,
        LINEARVELOCITY = 32,
        LINEARFORCE = 64,
        BROADPHASE = 128,
        BROADPHASE_PAIRS = 256,
        BOUNDING_RADIUS = 512,
    };

    class Constraint;
    class TimeStep;

    class LUMOS_EXPORT LumosPhysicsEngine : public ISystem
    {
    public:
        LumosPhysicsEngine();
        ~LumosPhysicsEngine();

        void SetDefaults();

        //Add Constraints
        void AddConstraint(Constraint* c)
        {
            m_Constraints.push_back(c);
        }

        void OnInit() override {};
        //Update Physics Engine
        void OnUpdate(const TimeStep& timeStep, Scene* scene) override;

        //Getters / Setters
        bool IsPaused() const
        {
            return m_IsPaused;
        }
        void SetPaused(bool paused)
        {
            m_IsPaused = paused;
        }

        const Maths::Vector3& GetGravity() const
        {
            return m_Gravity;
        }
        void SetGravity(const Maths::Vector3& g)
        {
            m_Gravity = g;
        }

        float GetDampingFactor() const
        {
            return m_DampingFactor;
        }
        void SetDampingFactor(float d)
        {
            m_DampingFactor = d;
        }

        static float GetDeltaTime()
        {
            return s_UpdateTimestep;
        }

        SharedRef<Broadphase> GetBroadphase() const
        {
            return m_BroadphaseDetection;
        }
        inline void SetBroadphase(const SharedRef<Broadphase>& bp)
        {
            m_BroadphaseDetection = bp;
        }

        int GetNumberCollisionPairs() const
        {
            return static_cast<int>(m_BroadphaseCollisionPairs.size());
        }
        int GetNumberRigidBodys() const
        {
            return static_cast<int>(m_RigidBodys.size());
        }

        IntegrationType GetIntegrationType() const
        {
            return m_IntegrationType;
        }
        void SetIntegrationType(const IntegrationType& type)
        {
            m_IntegrationType = type;
        }

        void ClearConstraints();

        void OnImGui() override;
        void OnDebugDraw() override;

        void SetDebugDrawFlags(uint32_t flags)
        {
            m_DebugDrawFlags = flags;
        }
        uint32_t GetDebugDrawFlags() const
        {
            return m_DebugDrawFlags;
        }

    protected:
        //The actual time-independant update function
        void UpdatePhysics();

        //Handles broadphase collision detection
        void BroadPhaseCollisions();

        //Handles narrowphase collision detection
        void NarrowPhaseCollisions();

        //Updates all Rigid Body position, orientation, velocity etc (default method uses symplectic euler integration)
        void UpdateRigidBodys();
        void UpdateRigidBody(RigidBody3D* obj) const;

        //Solves all engine constraints (constraints and manifolds)
        void SolveConstraints();

    protected:
        bool m_IsPaused;
        float m_UpdateAccum;
        Maths::Vector3 m_Gravity;
        float m_DampingFactor;

        std::vector<RigidBody3D*> m_RigidBodys;
        std::vector<CollisionPair> m_BroadphaseCollisionPairs;

        std::vector<Constraint*> m_Constraints; // Misc constraints between pairs of objects
        std::vector<Manifold> m_Manifolds; // Contact constraints between pairs of objects
        std::mutex m_ManifoldsMutex;

        SharedRef<Broadphase> m_BroadphaseDetection;
        IntegrationType m_IntegrationType;

        uint32_t m_DebugDrawFlags = 0;
        std::mutex m_ManifoldLock;

        bool m_MultipleUpdates = true;
        static float s_UpdateTimestep;
    };
}
