#pragma once

#include "Utilities/TSingleton.h"
#include "RigidBody3D.h"
#include "Narrowphase/Manifold.h"
#include "Broadphase/Broadphase.h"
#include "Scene/ISystem.h"
#include "Core/OS/Allocators/PoolAllocator.h"

namespace Lumos
{
    enum class LUMOS_EXPORT IntegrationType : uint32_t
    {
        EXPLICIT_EULER      = 0,
        SEMI_IMPLICIT_EULER = 1,
        RUNGE_KUTTA_2       = 2,
        RUNGE_KUTTA_4       = 3
    };

    enum class LUMOS_EXPORT BroadphaseType : uint32_t
    {
        BRUTE_FORCE    = 0,
        SORT_AND_SWEAP = 1,
        OCTREE         = 2,
    };

    enum PhysicsDebugFlags : uint32_t
    {
        CONSTRAINT       = 1,
        MANIFOLD         = 2,
        COLLISIONVOLUMES = 4,
        COLLISIONNORMALS = 8,
        AABB             = 16,
        LINEARVELOCITY   = 32,
        LINEARFORCE      = 64,
        BROADPHASE       = 128,
        BROADPHASE_PAIRS = 256,
        BOUNDING_RADIUS  = 512,
    };

    class Constraint;
    class TimeStep;
    class Scene;

    struct PhysicsStats3D
    {
        uint32_t RigidBodyCount;
        uint32_t CollisionCount;
        uint32_t RestCount;
        uint32_t StaticCount;
        uint32_t ConstraintCount;
        uint32_t NarrowPhaseCount;
    };

    struct LumosPhysicsEngineConfig
    {
        float TimeStep             = 1.0f / 120.0f;
        uint32_t RigidBodyPool     = 10000;
        glm::vec3 Gravity          = glm::vec3(0.0f, -9.81f, 0.0f);
        float DampingFactor        = 0.9995f;
        IntegrationType IntegrType = IntegrationType::RUNGE_KUTTA_4;
    };

    class LUMOS_EXPORT LumosPhysicsEngine : public ISystem
    {
    public:
        LumosPhysicsEngine(const LumosPhysicsEngineConfig& config = {});
        ~LumosPhysicsEngine();

        void SetDefaults();

        bool OnInit() override { return true; };
        // Update Physics Engine
        void OnUpdate(const TimeStep& timeStep, Scene* scene) override;

        void SyncTransforms(Scene* scene);

        // Getters / Setters
        bool IsPaused() const { return m_IsPaused; }
        void SetPaused(bool paused) { m_IsPaused = paused; }

        const glm::vec3& GetGravity() const { return m_Gravity; }
        void SetGravity(const glm::vec3& g) { m_Gravity = g; }

        float GetDampingFactor() const { return m_DampingFactor; }
        void SetDampingFactor(float d) { m_DampingFactor = d; }

        static float GetDeltaTime() { return s_UpdateTimestep; }
        SharedPtr<Broadphase> GetBroadphase() const { return m_BroadphaseDetection; }

        inline void SetBroadphase(const SharedPtr<Broadphase>& bp) { m_BroadphaseDetection = bp; }
        int GetNumberCollisionPairs() const { return static_cast<int>(m_BroadphaseCollisionPairs.Size()); }
        int GetNumberRigidBodys() const { return static_cast<int>(m_Stats.RigidBodyCount); }
        IntegrationType GetIntegrationType() const { return m_IntegrationType; }
        void SetIntegrationType(const IntegrationType& type) { m_IntegrationType = type; }
        void SetBroadphaseType(BroadphaseType type);

        void ClearConstraints();

        void OnImGui() override;
        void OnDebugDraw() override;

        void SetDebugDrawFlags(uint32_t flags) { m_DebugDrawFlags = flags; }
        uint32_t GetDebugDrawFlags() const { return m_DebugDrawFlags; }

        std::string IntegrationTypeToString(IntegrationType type);
        std::string BroadphaseTypeToString(BroadphaseType type);

        uint32_t GetMaxUpdatesPerFrame() const { return m_MaxUpdatesPerFrame; }
        void SetMaxUpdatesPerFrame(uint32_t updates) { m_MaxUpdatesPerFrame = updates; }

        uint32_t GetVelocityIterations() const { return m_VelocityIterations; }
        void SetVelocityIterations(uint32_t iterations) { m_VelocityIterations = iterations; }

        uint32_t GetPositionIterations() const { return m_PositionIterations; }
        void SetPositionIterations(uint32_t iterations) { m_PositionIterations = iterations; }

        RigidBody3D* CreateBody(const RigidBody3DProperties& properties = {});
        void DestroyBody(RigidBody3D* body);

        const PhysicsStats3D& GetStats() const { return m_Stats; }

    protected:
        // The actual time-independant update function
        void UpdatePhysics();

        // Handles broadphase collision detection
        void BroadPhaseCollisions();

        // Handles narrowphase collision detection
        void NarrowPhaseCollisions();

        // Updates all Rigid Body position, orientation, velocity etc (default method uses symplectic euler integration)
        void UpdateRigidBodys();
        void UpdateRigidBody(RigidBody3D* obj) const;

        // Solves all engine constraints (constraints and manifolds)
        void SolveConstraints();

    protected:
        bool m_IsPaused;
        float m_UpdateAccum;
        glm::vec3 m_Gravity;
        float m_DampingFactor;
        uint32_t m_MaxUpdatesPerFrame = 5;
        uint32_t m_PositionIterations = 2;
        uint32_t m_VelocityIterations = 10;

        Vector<CollisionPair> m_BroadphaseCollisionPairs;
        SharedPtr<Constraint>* m_Constraints; // Misc constraints between pairs of objects
        Manifold* m_Manifolds;                // Contact constraints between pairs of objects
        std::mutex m_ManifoldsMutex;

        uint32_t m_ManifoldCount   = 0;
        uint32_t m_ConstraintCount = 0;

        SharedPtr<Broadphase> m_BroadphaseDetection;
        BroadphaseType m_BroadphaseType;
        IntegrationType m_IntegrationType;

        uint32_t m_DebugDrawFlags = 0;
        std::mutex m_ManifoldLock;

        RigidBody3D* m_RootBody;
        PoolAllocator<RigidBody3D>* m_Allocator;
        Arena* m_Arena;

        PhysicsStats3D m_Stats;

        static float s_UpdateTimestep;
    };
}
