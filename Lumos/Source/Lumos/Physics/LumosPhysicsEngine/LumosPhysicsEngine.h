#pragma once

#include "Utilities/TSingleton.h"
#include "Narrowphase/Manifold.h"
#include "RigidBody3D.h"
#include "RaycastResult.h"
#include "Broadphase/Broadphase.h"
#include "Scene/ISystem.h"
#include "Core/OS/Allocators/PoolAllocator.h"
#include "Core/Mutex.h"


namespace Lumos
{
    class RigidBody3D;

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

    // One fixed substep, in the order it runs.
    enum PhysicsStage : uint32_t
    {
        PhysicsStage_Broadphase = 0,
        PhysicsStage_Narrowphase,
        PhysicsStage_Forces,
        PhysicsStage_Solve,
        PhysicsStage_Integrate,
        PhysicsStage_Rest,
        PhysicsStage_Count
    };

    class Constraint;
    class TimeStep;
    class Scene;
    class LuaScriptComponent;
    class RaycastVehicle;

    struct CollisionEvent3D
    {
        RigidBody3D* BodyA;
        RigidBody3D* BodyB;
        CollisionInfo3D InfoA; // Info from A's perspective (OtherBody = B)
        CollisionInfo3D InfoB; // Info from B's perspective (OtherBody = A)
    };

    struct CollisionPairKey
    {
        RigidBody3D* A;
        RigidBody3D* B;
        bool operator==(const CollisionPairKey& other) const
        {
            return (A == other.A && B == other.B) || (A == other.B && B == other.A);
        }
    };

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
        Vec3 Gravity               = Vec3(0.0f, -9.81f, 0.0f);
        float DampingFactor        = 0.99995f;
        IntegrationType IntegrType = IntegrationType::RUNGE_KUTTA_2;
        float BaumgarteScalar      = 0.3f;   // Amount of force to add to the System to solve error
        float BaumgarteSlop        = 0.001f; // Amount of allowed penetration, ensures a complete manifold each frame
        float PenetrationSlop      = 0.02f;  // How much bodies are allowed to sink into each other in meters
        u32 MaxRigidBodyCount      = 4096;
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

        const Vec3& GetGravity() const { return m_Gravity; }
        void SetGravity(const Vec3& g) { m_Gravity = g; }

        float GetDampingFactor() const { return m_DampingFactor; }
        void SetDampingFactor(float d) { m_DampingFactor = d; }

        float GetTimeScale() const { return m_TimeScale; }
        void SetTimeScale(float scale) { m_TimeScale = scale; }

        bool GetParallelNarrowphase() const { return m_ParallelNarrowphase; }
        void SetParallelNarrowphase(bool parallel) { m_ParallelNarrowphase = parallel; }

        static float GetDeltaTime() { return s_UpdateTimestep; }
        SharedPtr<Broadphase> GetBroadphase() const { return m_BroadphaseDetection; }

        inline void SetBroadphase(const SharedPtr<Broadphase>& bp) { m_BroadphaseDetection = bp; }
        int GetNumberCollisionPairs() const { return static_cast<int>(m_BroadphaseCollisionPairs.Size()); }
        int GetNumberRigidBodys() const { return static_cast<int>(m_Stats.RigidBodyCount); }
        IntegrationType GetIntegrationType() const { return m_IntegrationType; }
        void SetIntegrationType(const IntegrationType& type) { m_IntegrationType = type; }
        void SetBroadphaseType(BroadphaseType type);
        BroadphaseType GetBroadphaseType() const { return m_BroadphaseType; }

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

        // Raycasting
        RaycastHit Raycast(const RaycastQuery& query) const;
        RaycastHit Raycast(const Vec3& origin, const Vec3& direction, float maxDistance = 1000.0f, uint16_t layerMask = 0xFFFF) const;
        bool RaycastAll(const RaycastQuery& query, TDArray<RaycastHit>& results, uint32_t maxResults = 32) const;
        bool RaycastBody(RigidBody3D& body, const RaycastQuery& query, const Vec3& invDir, float maxDistance, RaycastHit& result) const;

        // Sweep fast bodies so they cannot pass through thin ones between steps.
        void SetContinuousCollision(bool enabled) { m_ContinuousCollision = enabled; }
        bool GetContinuousCollision() const { return m_ContinuousCollision; }

        // Warm-start tuning
        void SetWarmStartingEnabled(bool enabled) { m_WarmStartingEnabled = enabled; }
        bool GetWarmStartingEnabled() const { return m_WarmStartingEnabled; }

        // Manual stepping: the simulation only advances by the stages asked for,
        // so a substep can be walked through one stage at a time.
        void SetManualStep(bool manual);
        bool GetManualStep() const { return m_ManualStep; }
        void StepStages(uint32_t count) { m_PendingStages += count; }
        // Finish the substep in progress, or run a whole one.
        void StepSubstep() { m_PendingStages += PhysicsStage_Count - m_NextStage; }
        PhysicsStage GetNextStage() const { return (PhysicsStage)m_NextStage; }
        // Last stage that ran, PhysicsStage_Count before the first step.
        PhysicsStage GetLastStage() const { return (PhysicsStage)m_LastStage; }
        float GetStageMs(PhysicsStage stage) const { return m_StageMs[stage]; }
        uint32_t GetManifoldCount() const { return m_ManifoldCount; }

        RaycastVehicle* CreateVehicle(RigidBody3D* chassis);
        void DestroyVehicle(RaycastVehicle* vehicle);

    protected:
        // The actual time-independant update function
        void UpdatePhysics();
        void RunStage(PhysicsStage stage);
        void ManualUpdate(Scene* scene);
        // Per-frame constraint list. False = nothing awake, nothing to simulate.
        bool GatherConstraints(Scene* scene);

        // Handles broadphase collision detection
        void BroadPhaseCollisions();

        // Handles narrowphase collision detection
        void NarrowPhaseCollisions();
        void NarrowPhaseCollisionsParallel();

        // Updates all Rigid Body position, orientation, velocity etc (default method uses symplectic euler integration)
        void UpdateRigidBodies();
        void UpdateRigidBody(RigidBody3D* obj) const;
        void StopAtFirstContact(RigidBody3D* obj, const Vec3& startPosition, const Quat& startOrientation, float dt) const;

        // Solves all engine constraints (constraints and manifolds)
        void SolveConstraints();

        void ApplyImpulses();

        void WarmStartManifolds();
        void SavePersistentImpulses();

        // Dispatch collision callbacks to Lua scripts
        void DispatchCollisionCallbacks(Scene* scene);

    protected:
        bool m_IsPaused;
        float m_UpdateAccum;
        Vec3 m_Gravity;
        float m_DampingFactor;
        uint32_t m_MaxUpdatesPerFrame = 5;
        uint32_t m_PositionIterations = 2;
        uint32_t m_VelocityIterations = 8;
        float m_TimeScale             = 1.0f;
        bool m_ParallelNarrowphase    = true;
        bool m_ParallelSolver         = true; // Solve independent contact islands on the job system.

        bool m_ContinuousCollision          = true;
        bool m_ManualStep                   = false;
        Scene* m_ManualScene                = nullptr;
        uint32_t m_PendingStages            = 0;
        uint32_t m_NextStage                = 0;
        uint32_t m_LastStage                = PhysicsStage_Count;
        float m_StageMs[PhysicsStage_Count] = { 0.0f };

        float m_BaumgarteScalar = 0.2f;   // Amount of force to add to the System to solve error
        float m_BaumgarteSlop   = 0.001f; // Amount of allowed penetration, ensures a complete manifold each frame

        TDArray<CollisionPair> m_BroadphaseCollisionPairs;
        SharedPtr<Constraint>* m_Constraints; // Misc constraints between pairs of objects
        Manifold* m_Manifolds;                // Contact constraints between pairs of objects

        u32 m_ManifoldCount   = 0;
        u32 m_MaxManifolds    = 0;
        u32 m_ConstraintCount = 0;
        u32 m_RigidBodyCount  = 0;

        SharedPtr<Broadphase> m_BroadphaseDetection;
        BroadphaseType m_BroadphaseType;
        IntegrationType m_IntegrationType;

        uint32_t m_DebugDrawFlags = 0;
        Mutex* m_ManifoldLock;

        RigidBody3D* m_RigidBodies;
        TDArray<RigidBody3D*> m_RigidBodyFreeList;
        u32 m_MaxRigidBodyCount;

        Arena* m_Arena;
        Arena* m_FrameArena;

        PhysicsStats3D m_Stats;

        // Collision event tracking for Lua callbacks
        TDArray<CollisionEvent3D> m_CollisionEvents;
        TDArray<CollisionPairKey> m_PrevCollisionPairs;
        TDArray<CollisionPairKey> m_CurrCollisionPairs;

        static float s_UpdateTimestep;

        static constexpr int kRollingBufferSize    = 60;
        float m_OverrunHistory[kRollingBufferSize] = { 0.0f };
        int m_OverrunIndex                         = 0;
        float m_AvgOverrun                         = 0.0f;

        struct PersistentContactPoint
        {
            Vec3 localPosA       = Vec3(0.0f); // contact pos in A's local frame
            Vec3 localPosB       = Vec3(0.0f); // contact pos in B's local frame
            Vec3 normalImpulse   = Vec3(0.0f); // world-space accumulated normal impulse
            Vec3 frictionImpulse = Vec3(0.0f); // world-space accumulated friction impulse
            Vec3 rollingImpulse  = Vec3(0.0f); // world-space accumulated rolling-friction angular impulse
        };

        struct PersistentManifoldCache
        {
            static constexpr int kMaxPersistedContacts = 16;
            PersistentContactPoint contacts[kMaxPersistedContacts];
            const RigidBody3D* savedNodeA = nullptr;
            int count                     = 0;
        };

        // Open-addressed key -> cache. Each substep's contacts are written into the spare
        // table and the two swap, so separated pairs drop out with nothing to erase.
        struct PersistentTable
        {
            TDArray<uint64_t> Keys; // 0 = empty slot
            TDArray<PersistentManifoldCache> Values;
            uint32_t Count = 0;

            void Reset(uint32_t expected);
            PersistentManifoldCache* Find(uint64_t key);
            PersistentManifoldCache* Insert(uint64_t key);
        };
        PersistentTable m_PersistentTables[2];
        uint32_t m_PersistentCurrent = 0; // the table WarmStartManifolds reads
        bool m_WarmStartingEnabled = true;

        TDArray<RaycastVehicle*> m_Vehicles;
    };
}
