#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "RigidBody3D.h"
#include "Narrowphase/CollisionDetection.h"
#include "Broadphase/BruteForceBroadphase.h"
#include "Broadphase/OctreeBroadphase.h"
#include "Integration.h"
#include "Constraints/Constraint.h"
#include "CollisionShapes/SphereCollisionShape.h"
#include "CollisionShapes/CapsuleCollisionShape.h"
#include "CollisionShapes/TerrainCollisionShape.h"
#include "Scene/Component/ModelComponent.h"
#include "Graphics/Model.h"
#include "Graphics/Terrain.h"
#include "Utilities/TimeStep.h"
#include "Core/OS/Window.h"
#include "Core/JobSystem.h"
#include "Core/Application.h"
#include "Scene/Component/RigidBody3DComponent.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Maths/MathsUtilities.h"
#include "Maths/Transform.h"
#include "ImGui/ImGuiUtilities.h"
#include "Utilities/Colour.h"

#include <entt/entt.hpp>
#include <imgui/imgui.h>

namespace Lumos
{

    float LumosPhysicsEngine::s_UpdateTimestep = 1.0f / 60.0f;

    LumosPhysicsEngine::LumosPhysicsEngine(const LumosPhysicsEngineConfig& config)
        : m_IsPaused(true)
        , m_UpdateAccum(0.0f)
        , m_Gravity(config.Gravity)
        , m_DampingFactor(config.DampingFactor)
        , m_BroadphaseDetection(nullptr)
        , m_IntegrationType(config.IntegrType)
        , m_BaumgarteScalar(config.BaumgarteScalar)
        , m_BaumgarteSlop(config.BaumgarteSlop)
        , m_MaxRigidBodyCount(config.MaxRigidBodyCount)
    {
        s_UpdateTimestep = config.TimeStep;
        m_DebugName = "Lumos3DPhysicsEngine";
        m_BroadphaseCollisionPairs.Reserve(1000);

        m_FrameArena        = ArenaAlloc(Megabytes(8));
        m_Arena             = ArenaAlloc(m_MaxRigidBodyCount * sizeof(RigidBody3D) * 2 + sizeof(Mutex));
        m_RigidBodies       = PushArray(m_Arena, RigidBody3D, m_MaxRigidBodyCount);
        m_RigidBodyFreeList = TDArray<RigidBody3D*>(m_Arena);
        m_RigidBodyFreeList.Reserve(m_MaxRigidBodyCount);

        m_ManifoldLock = PushArray(m_Arena, Mutex, 1);
        MutexInit(m_ManifoldLock);
    }

    void LumosPhysicsEngine::SetDefaults()
    {
        m_IsPaused        = true;
        s_UpdateTimestep  = 1.0f / 120.0f;
        m_UpdateAccum     = 0.0f;
        m_Gravity         = Vec3(0.0f, -9.81f, 0.0f);
        m_DampingFactor   = 0.9995f;
        m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
        m_BaumgarteScalar = 0.3f;
        m_BaumgarteSlop   = 0.001f;
    }

    LumosPhysicsEngine::~LumosPhysicsEngine()
    {
        MutexDestroy(m_ManifoldLock);

        ArenaRelease(m_Arena);
        ArenaRelease(m_FrameArena);

        CollisionDetection::Release();
    }

    void LumosPhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_IsPaused)
        {
            auto& registry    = scene->GetRegistry();
            m_ConstraintCount = 0;
            ArenaClear(m_FrameArena);

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Spring Constraints");
                auto viewSpring = registry.view<SpringConstraintComponent>();
                auto viewAxis   = registry.view<AxisConstraintComponent>();
                auto viewDis    = registry.view<DistanceConstraintComponent>();
                auto viewWeld   = registry.view<WeldConstraintComponent>();

                m_ConstraintCount = (uint32_t)viewSpring.size() + (uint32_t)viewAxis.size() + (uint32_t)viewDis.size() + (uint32_t)viewWeld.size();

                // Early-out if no dynamic awake bodies and no constraints
                if(m_ConstraintCount == 0)
                {
                    bool hasActiveBodies = false;
                    for(u32 i = 0; i < m_RigidBodyCount; i++)
                    {
                        if(m_RigidBodies[i].m_IsValid && !m_RigidBodies[i].GetIsStatic() && m_RigidBodies[i].IsAwake())
                        {
                            hasActiveBodies = true;
                            break;
                        }
                    }
                    if(!hasActiveBodies)
                        return;
                }
                m_Constraints     = PushArray(m_FrameArena, SharedPtr<Constraint>, m_ConstraintCount);

                uint32_t constraintIndex = 0;
                for(auto entity : viewSpring)
                {
                    auto& springComp = viewSpring.get<SpringConstraintComponent>(entity);

                    if(!springComp.Initialised())
                        springComp.Initialise();

                    if(springComp.Initialised())
                        m_Constraints[constraintIndex++] = viewSpring.get<SpringConstraintComponent>(entity).GetConstraint();
                }

                for(auto entity : viewAxis)
                {
                    auto constraint = viewAxis.get<AxisConstraintComponent>(entity);

                    if(constraint.GetEntityID() != Entity(entity, Application::Get().GetCurrentScene()).GetID())
                        constraint.SetEntity(Entity(entity, Application::Get().GetCurrentScene()).GetID());
                    if(constraint.GetConstraint())
                        m_Constraints[constraintIndex++] = constraint.GetConstraint();
                }

                for(auto entity : viewDis)
                {
                    m_Constraints[constraintIndex++] = viewDis.get<DistanceConstraintComponent>(entity).GetConstraint();
                }

                for(auto entity : viewWeld)
                {
                    m_Constraints[constraintIndex++] = viewWeld.get<WeldConstraintComponent>(entity).GetConstraint();
                }
            }

            m_Stats.ConstraintCount = m_ConstraintCount;

            {
                LUMOS_PROFILE_SCOPE("Physics::UpdatePhysics");

                m_CollisionEvents.Clear();
                m_CurrCollisionPairs.Clear();

                m_UpdateAccum += (float)timeStep.GetSeconds() * m_TimeScale;
                for(uint32_t i = 0; (m_UpdateAccum + Maths::M_EPSILON >= s_UpdateTimestep) && i < m_MaxUpdatesPerFrame; ++i)
                {
                    m_UpdateAccum -= s_UpdateTimestep;
                    UpdatePhysics();
                }

                // Dispatch Lua collision callbacks after physics
                DispatchCollisionCallbacks(scene);

                // Track collision end: pairs in prev but not in curr
                // Swap prev/curr for next frame
                Swap(m_PrevCollisionPairs, m_CurrCollisionPairs);

                float overrun = 0.0f;
                if(m_UpdateAccum + Maths::M_EPSILON >= s_UpdateTimestep)
                {
                    overrun       = m_UpdateAccum;
                    m_UpdateAccum = Maths::Mod(overrun, s_UpdateTimestep);
                }

                m_OverrunHistory[m_OverrunIndex] = overrun;
                m_OverrunIndex                   = (m_OverrunIndex + 1) % kRollingBufferSize;

                float sum = 0.0f;
                for(int i = 0; i < kRollingBufferSize; ++i)
                    sum += m_OverrunHistory[i];
                m_AvgOverrun = sum / kRollingBufferSize;

            }
        }
    }

    void LumosPhysicsEngine::UpdatePhysics()
    {
        m_ManifoldCount = 0;
        m_MaxManifolds  = 1000;
        m_Manifolds     = PushArrayNoZero(m_FrameArena, Manifold, m_MaxManifolds);

        // Check for collisions
        BroadPhaseCollisions();

        if(m_ParallelNarrowphase && m_BroadphaseCollisionPairs.Size() > 32)
            NarrowPhaseCollisionsParallel();
        else
            NarrowPhaseCollisions();

        // Solve collision constraints
        SolveConstraints();
        // Update movement
        for(uint32_t i = 0; i < m_PositionIterations; i++)
            UpdateRigidBodies();

        for(u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& current = m_RigidBodies[i];
            if(current.m_IsValid)
                current.RestTest();
        }
    }

    void LumosPhysicsEngine::UpdateRigidBodies()
    {
        LUMOS_PROFILE_SCOPE("Update Rigid Body");

        m_Stats.StaticCount    = 0;
        m_Stats.RestCount      = 0;
        m_Stats.RigidBodyCount = 0;

        for(u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& current = m_RigidBodies[i];
            if(!current.m_IsValid)
                continue;

            if(current.m_AtRest)
                m_Stats.RestCount++;
            if(current.m_Static)
                m_Stats.StaticCount++;

            m_Stats.RigidBodyCount++;

            UpdateRigidBody(&current);
        }
    }

    RigidBody3D* LumosPhysicsEngine::CreateBody(const RigidBody3DProperties& properties)
    {
        if(!m_RigidBodyFreeList.Empty())
        {
            RigidBody3D* body = m_RigidBodyFreeList.Back();
            *body             = RigidBody3D(properties);
            body->m_IsValid   = true;
            m_RigidBodyFreeList.PopBack();

            return body;
        }
        else
        {
            if(m_RigidBodyCount < m_MaxRigidBodyCount)
            {
                RigidBody3D* body = &m_RigidBodies[m_RigidBodyCount];
                *body             = RigidBody3D(properties);
                body->m_IsValid   = true;
                m_RigidBodyCount++;

                return body;
            }
            else
            {
                LERROR("Exceeded Max RigidBody Count %i", m_RigidBodyCount);
                return nullptr;
            }
        }
    }

    void LumosPhysicsEngine::DestroyBody(RigidBody3D* body)
    {
        if(body && body->m_IsValid)
        {
            body->m_IsValid = false;
            m_RigidBodyFreeList.PushBack(body);
        }
    }

    void LumosPhysicsEngine::SyncTransforms(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();

        if(!scene)
            return;

        auto& registry = scene->GetRegistry();
        auto group     = registry.group<RigidBody3DComponent>(entt::get<Maths::Transform>);

        for(auto entity : group)
        {
            const auto& [phys, trans] = group.get<RigidBody3DComponent, Maths::Transform>(entity);

            if(!phys.GetRigidBody()->GetIsStatic() && phys.GetRigidBody()->IsAwake())
            {
                ASSERT(phys.GetRigidBody()->GetPosition().IsValid());
                ASSERT(phys.GetRigidBody()->GetOrientation().IsValid());

                trans.SetLocalPosition(phys.GetRigidBody()->GetPosition());
                trans.SetLocalOrientation(phys.GetRigidBody()->GetOrientation());
            }
        };

        // Auto-attach TerrainCollisionShape to terrain entities missing collision data
        {
            auto terrainView = registry.view<Graphics::ModelComponent, RigidBody3DComponent>();
            for(auto entity : terrainView)
            {
                auto& model = terrainView.get<Graphics::ModelComponent>(entity);
                auto& phys  = terrainView.get<RigidBody3DComponent>(entity);
                auto* body  = phys.GetRigidBody();

                if(!body || !model.ModelRef)
                    continue;
                if(model.ModelRef->GetPrimitiveType() != Graphics::PrimitiveType::Terrain)
                    continue;

                // Skip if already has a terrain collision shape with populated height data
                auto existingShape = body->GetCollisionShape();
                if(existingShape && existingShape->GetType() == CollisionShapeType::CollisionTerrain)
                {
                    auto* tcs = static_cast<TerrainCollisionShape*>(existingShape.get());
                    if(tcs->HasHeightData())
                        continue;
                }

                // Find terrain mesh
                auto& meshes = model.ModelRef->GetMeshes();
                if(meshes.Empty())
                    continue;

                Terrain* terrain = dynamic_cast<Terrain*>(meshes[0].get());
                if(!terrain || terrain->GetHeightData().Empty())
                    continue;

                // Create and populate TerrainCollisionShape
                auto terrainShape = CreateSharedPtr<TerrainCollisionShape>();
                terrainShape->SetHeightData(
                    (uint32_t)terrain->GetGridWidth(),
                    (uint32_t)terrain->GetGridHeight(),
                    const_cast<float*>(terrain->GetHeightData().Data()),
                    terrain->GetXScale(),
                    terrain->GetYScale());

                body->SetCollisionShape(terrainShape);
                body->SetIsStatic(true);
            }
        }
    }

    Quat QuatMulVec3(const Quat& quat, const Vec3& b)
    {
        Quat ans;

        ans.w = -(quat.x * b.x) - (quat.y * b.y) - (quat.z * b.z);

        ans.x = (quat.w * b.x) + (b.y * quat.z) - (b.z * quat.y);
        ans.y = (quat.w * b.y) + (b.z * quat.x) - (b.x * quat.z);
        ans.z = (quat.w * b.z) + (b.x * quat.y) - (b.y * quat.x);

        return ans;
    }

    void LumosPhysicsEngine::UpdateRigidBody(RigidBody3D* obj) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        // Use local variable instead of modifying static timestep
        const float dt = s_UpdateTimestep / m_PositionIterations;

        if(!obj->GetIsStatic() && obj->IsAwake())
        {
            const float damping = m_DampingFactor;

            // Apply gravity
            if(obj->m_InvMass > 0.0f)
                obj->m_LinearVelocity += m_Gravity * dt;

            switch(m_IntegrationType)
            {
            case IntegrationType::EXPLICIT_EULER:
            {
                // Update position
                obj->m_Position += obj->m_LinearVelocity * dt;

                // Update linear velocity (v = u + at)
                obj->m_LinearVelocity += obj->m_Force * obj->m_InvMass * dt;

                // Linear velocity damping + factor
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;
                obj->m_LinearVelocity *= obj->m_LinearFactor;

                // Update orientation
                obj->m_Orientation += obj->m_Orientation * Quat(obj->m_AngularVelocity * dt);
                obj->m_Orientation.Normalise();

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * dt;

                // Angular velocity damping + factor
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;
                obj->m_AngularVelocity *= obj->m_AngularFactor;

                break;
            }

            case IntegrationType::SEMI_IMPLICIT_EULER:
            {
                // Update linear velocity (v = u + at)
                obj->m_LinearVelocity += obj->m_Force * obj->m_InvMass * dt;

                // Linear velocity damping + factor
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;
                obj->m_LinearVelocity *= obj->m_LinearFactor;

                // Update position
                obj->m_Position += obj->m_LinearVelocity * dt;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * dt;

                // Angular velocity damping + factor
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;
                obj->m_AngularVelocity *= obj->m_AngularFactor;

                auto angularVelocity = obj->m_AngularVelocity * dt;

                // Update orientation
                obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_2:
            {
                // RK2 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK2(state, 0.0f, dt);

                obj->m_Position       = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping + factor
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;
                obj->m_LinearVelocity *= obj->m_LinearFactor;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * dt;

                // Angular velocity damping + factor
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;
                obj->m_AngularVelocity *= obj->m_AngularFactor;

                auto angularVelocity = obj->m_AngularVelocity * dt * 0.5f;

                // Update orientation
                obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_4:
            {
                // RK4 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK4(state, 0.0f, dt);
                obj->m_Position       = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping + factor
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;
                obj->m_LinearVelocity *= obj->m_LinearFactor;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * dt;

                // Angular velocity damping + factor
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;
                obj->m_AngularVelocity *= obj->m_AngularFactor;

                // Update orientation
                auto angularVelocity = obj->m_AngularVelocity * dt * 0.5f;

                obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                obj->m_Orientation.Normalise();

                break;
            }
            }

            // Mark cached world transform and AABB as invalid
            obj->m_WSTransformInvalidated = true;
            obj->m_WSAabbInvalidated      = true;
        }

        ASSERT(obj->m_Orientation.IsValid());
        ASSERT(obj->m_Position.IsValid());
    }

    Quat AngularVelcityToQuaternion(const Vec3& angularVelocity)
    {
        Quat q;
        q.x = 0.5f * angularVelocity.x;
        q.y = 0.5f * angularVelocity.y;
        q.z = 0.5f * angularVelocity.z;
        q.w = 0.5f * Maths::Length(angularVelocity);
        return q;
    }

    void LumosPhysicsEngine::BroadPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        m_BroadphaseCollisionPairs.Clear();
        if(m_BroadphaseDetection)
            m_BroadphaseDetection->FindPotentialCollisionPairs(m_RigidBodies, m_BroadphaseCollisionPairs, m_RigidBodyCount);

#ifdef CHECK_COLLISION_PAIR_DUPLICATES

        uint32_t duplicatePairs = 0;
        for(size_t i = 0; i < m_BroadphaseCollisionPairs.Size(); ++i)
        {
            auto& pair = m_BroadphaseCollisionPairs[i];
            for(size_t j = i + 1; j < m_BroadphaseCollisionPairs.Size(); ++j)
            {
                auto& pair2 = m_BroadphaseCollisionPairs[j];
                if(pair.pObjectA == pair2.pObjectA && pair.pObjectB == pair2.pObjectB)
                {
                    duplicatePairs++;
                }
                else if(pair.pObjectA == pair2.pObjectB && pair.pObjectB == pair2.pObjectA)
                {
                    duplicatePairs++;
                }
            }
        }

        LINFO(duplicatePairs);
#endif
    }

    void LumosPhysicsEngine::NarrowPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_BroadphaseCollisionPairs.Empty())
            return;

        m_Stats.NarrowPhaseCount = (uint32_t)m_BroadphaseCollisionPairs.Size();
        m_Stats.CollisionCount   = 0;
        CollisionData colData;

        for(auto& cp : m_BroadphaseCollisionPairs)
        {
            auto shapeA = cp.pObjectA->GetCollisionShape();
            auto shapeB = cp.pObjectB->GetCollisionShape();

            if(shapeA && shapeB)
            {

                // Broadphase debug draw
                if(m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE_PAIRS)
                {
                    Vec4 colour = Colour::RandomColour();
                    DebugRenderer::DrawThickLine(cp.pObjectA->GetPosition(), cp.pObjectB->GetPosition(), 0.02f, false, colour);
                    DebugRenderer::DrawPoint(cp.pObjectA->GetPosition(), 0.05f, false, colour);
                    DebugRenderer::DrawPoint(cp.pObjectB->GetPosition(), 0.05f, false, colour);
                }

                // Detects if the objects are colliding - Seperating Axis Theorem
                if(CollisionDetection::Get().CheckCollision(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), &colData))
                {
                    // Queue collision event for Lua dispatch
                    {
                        CollisionEvent3D evt;
                        evt.BodyA = cp.pObjectA;
                        evt.BodyB = cp.pObjectB;
                        evt.InfoA = { cp.pObjectB, colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectB->GetIsTrigger() };
                        evt.InfoB = { cp.pObjectA, -colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectA->GetIsTrigger() };
                        m_CollisionEvents.PushBack(evt);
                        m_CurrCollisionPairs.PushBack({ cp.pObjectA, cp.pObjectB });
                    }

                    // Check to see if any of the objects have collision callbacks that dont
                    // want the objects to physically collide
                    const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
                    const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

                    if(okA && okB)
                    {
                        // Skip physics response for triggers (they only fire callbacks)
                        if(cp.pObjectA->GetIsTrigger() || cp.pObjectB->GetIsTrigger())
                            continue;

                        // Check if we have space for another manifold
                        if(m_ManifoldCount >= m_MaxManifolds)
                        {
                            LWARN("Exceeded max manifold count (%u), skipping collision", m_MaxManifolds);
                            continue;
                        }

                        // Build full collision manifold that will also handle the collision
                        // response between the two objects in the solver stage
                        ScopedMutex lock(m_ManifoldLock);
                        Manifold& manifold = m_Manifolds[m_ManifoldCount++];
                        manifold.Initiate(cp.pObjectA, cp.pObjectB, m_BaumgarteScalar, m_BaumgarteSlop);
                        // Construct contact points that form the perimeter of the collision manifold
                        if(CollisionDetection::Get().BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, &manifold))
                        {
                            if(m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONNORMALS)
                            {
                                DebugRenderer::DrawPoint(colData.pointOnPlane, 0.1f, false, Vec4(0.5f, 0.5f, 1.0f, 1.0f), 3.0f);
                                DebugRenderer::DrawThickLine(colData.pointOnPlane, colData.pointOnPlane - colData.normal * colData.penetration, 0.05f, false, Vec4(0.0f, 0.0f, 1.0f, 1.0f), 3.0f);
                            }

                            // Fire callback
                            cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, &manifold);
                            cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, &manifold);
                            m_Stats.CollisionCount++;
                        }
                        else
                        {
                            m_ManifoldCount--;
                        }
                    }
                }
            }
        }
    }

    void LumosPhysicsEngine::NarrowPhaseCollisionsParallel()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_BroadphaseCollisionPairs.Empty())
            return;

        m_Stats.NarrowPhaseCount = (uint32_t)m_BroadphaseCollisionPairs.Size();
        std::atomic<uint32_t> collisionCount { 0 };

        const uint32_t pairCount = (uint32_t)m_BroadphaseCollisionPairs.Size();
        const uint32_t groupSize = 16; // Process 16 pairs per job

        System::JobSystem::Context ctx;
        System::JobSystem::Dispatch(ctx, pairCount, groupSize, [&](JobDispatchArgs args) {
            const uint32_t i = args.jobIndex;
            if(i >= pairCount)
                return;

            auto& cp    = m_BroadphaseCollisionPairs[i];
            auto shapeA = cp.pObjectA->GetCollisionShape();
            auto shapeB = cp.pObjectB->GetCollisionShape();

            if(!shapeA || !shapeB)
                return;

            CollisionData colData;
            if(!CollisionDetection::Get().CheckCollision(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), &colData))
                return;

            // Queue collision event for Lua dispatch (mutex-protected)
            {
                ScopedMutex lock(m_ManifoldLock);
                CollisionEvent3D evt;
                evt.BodyA = cp.pObjectA;
                evt.BodyB = cp.pObjectB;
                evt.InfoA = { cp.pObjectB, colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectB->GetIsTrigger() };
                evt.InfoB = { cp.pObjectA, -colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectA->GetIsTrigger() };
                m_CollisionEvents.PushBack(evt);
                m_CurrCollisionPairs.PushBack({ cp.pObjectA, cp.pObjectB });
            }

            // Collision callbacks (may not be thread-safe, but user's responsibility)
            const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
            const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

            if(!okA || !okB)
                return;

            // Skip triggers
            if(cp.pObjectA->GetIsTrigger() || cp.pObjectB->GetIsTrigger())
                return;

            // Thread-safe manifold creation
            {
                ScopedMutex lock(m_ManifoldLock);

                if(m_ManifoldCount >= m_MaxManifolds)
                    return;

                Manifold& manifold = m_Manifolds[m_ManifoldCount++];
                manifold.Initiate(cp.pObjectA, cp.pObjectB, m_BaumgarteScalar, m_BaumgarteSlop);

                if(CollisionDetection::Get().BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, &manifold))
                {
                    cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, &manifold);
                    cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, &manifold);
                    collisionCount.fetch_add(1);
                }
                else
                {
                    m_ManifoldCount--;
                }
            }
        });

        System::JobSystem::Wait(ctx);
        m_Stats.CollisionCount = collisionCount.load();
    }

    void LumosPhysicsEngine::SolveConstraints()
    {
        LUMOS_PROFILE_FUNCTION();

        {
            LUMOS_PROFILE_SCOPE("Solve Manifolds");
            for(uint32_t index = 0; index < m_ManifoldCount; index++)
                m_Manifolds[index].PreSolverStep(s_UpdateTimestep);
        }
        {
            LUMOS_PROFILE_SCOPE("Solve Constraints");
            for(uint32_t index = 0; index < m_ConstraintCount; index++)
                m_Constraints[index]->PreSolverStep(s_UpdateTimestep);
        }
        {
            LUMOS_PROFILE_SCOPE("Apply Impulses");

            for(uint32_t i = 0; i < m_VelocityIterations; i++)
            {
                for(uint32_t index = 0; index < m_ManifoldCount; index++)
                    m_Manifolds[index].ApplyImpulse();

                for(uint32_t index = 0; index < m_ConstraintCount; index++)
                    m_Constraints[index]->ApplyImpulse();
            }
        }
    }

    void LumosPhysicsEngine::ClearConstraints()
    {
        m_ConstraintCount = 0;
    }

    std::string LumosPhysicsEngine::IntegrationTypeToString(IntegrationType type)
    {
        switch(type)
        {
        case IntegrationType::EXPLICIT_EULER:
            return "EXPLICIT EULER";
        case IntegrationType::SEMI_IMPLICIT_EULER:
            return "SEMI IMPLICIT EULER";
        case IntegrationType::RUNGE_KUTTA_2:
            return "RUNGE KUTTA 2";
        case IntegrationType::RUNGE_KUTTA_4:
            return "RUNGE KUTTA 4";
        default:
            return "";
        }
    }

    std::string LumosPhysicsEngine::BroadphaseTypeToString(BroadphaseType type)
    {
        switch(type)
        {
        case BroadphaseType::BRUTE_FORCE:
            return "Brute Force";
        case BroadphaseType::SORT_AND_SWEAP:
            return "Sort and Sweap";
        case BroadphaseType::OCTREE:
            return "Octree";
        default:
            return "";
        }
    }

    void LumosPhysicsEngine::SetBroadphaseType(BroadphaseType type)
    {
        if(type == m_BroadphaseType)
            return;

        switch(type)
        {
        case BroadphaseType::SORT_AND_SWEAP:
        case BroadphaseType::BRUTE_FORCE:
            m_BroadphaseDetection = Lumos::CreateSharedPtr<BruteForceBroadphase>();
            break;
        case BroadphaseType::OCTREE:
            m_BroadphaseDetection = Lumos::CreateSharedPtr<OctreeBroadphase>(5, 8);
            break;
        default:
            m_BroadphaseDetection = Lumos::CreateSharedPtr<BruteForceBroadphase>();
            break;
        }

        m_BroadphaseType = type;
    }

    void LumosPhysicsEngine::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::TextUnformatted("3D Physics Engine");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Collision Pairs");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", GetNumberCollisionPairs());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        uint32_t maxCollisionPairs = Maths::nChoosek(m_Stats.RigidBodyCount, 2);
        ImGuiUtilities::Property("Max Number Of Collision Pairs", maxCollisionPairs, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Rigid Body Count", m_Stats.RigidBodyCount, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Static Body Count", m_Stats.StaticCount, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Rest Body Count", m_Stats.RestCount, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Collision Count", m_Stats.CollisionCount, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("NarrowPhase Count", m_Stats.NarrowPhaseCount, ImGuiUtilities::PropertyFlag::ReadOnly);
        ImGuiUtilities::Property("Constraint Count", m_Stats.ConstraintCount, ImGuiUtilities::PropertyFlag::ReadOnly);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Checkbox("##Paused", &m_IsPaused);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Gravity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat3("##Gravity", &m_Gravity.x);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Damping Factor");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##Damping Factor", &m_DampingFactor);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Integration Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(IntegrationTypeToString(m_IntegrationType).c_str()))
        {
            if(ImGui::MenuItem("EXPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 0, true))
            {
                m_IntegrationType = IntegrationType::EXPLICIT_EULER;
            }
            if(ImGui::MenuItem("SEMI IMPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 1, true))
            {
                m_IntegrationType = IntegrationType::SEMI_IMPLICIT_EULER;
            }
            if(ImGui::MenuItem("RUNGE KUTTA 2", "", static_cast<int>(m_IntegrationType) == 2, true))
            {
                m_IntegrationType = IntegrationType::RUNGE_KUTTA_2;
            }
            if(ImGui::MenuItem("RUNGE KUTTA 4", "", static_cast<int>(m_IntegrationType) == 3, true))
            {
                m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
            }
            ImGui::EndMenu();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);

        ImGui::Text("Avg Physics Overrun: %.4f", m_AvgOverrun);
        ImGui::ProgressBar(Maths::Clamp(m_AvgOverrun / s_UpdateTimestep, 0.0f, 1.0f));

        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void LumosPhysicsEngine::OnDebugDraw()
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        if(m_DebugDrawFlags & PhysicsDebugFlags::MANIFOLD)
        {
            for(uint32_t index = 0; index < m_ManifoldCount; index++)
                m_Manifolds[index].DebugDraw();
        }

        if(m_IsPaused)
            m_ManifoldCount = 0;

        // Draw all constraints
        if(m_DebugDrawFlags & PhysicsDebugFlags::CONSTRAINT)
        {
            for(uint32_t index = 0; index < m_ConstraintCount; index++)
                m_Constraints[index]->DebugDraw();
        }

        if(!m_IsPaused && m_BroadphaseDetection && (m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE))
            m_BroadphaseDetection->DebugDraw();

        for(u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& current = m_RigidBodies[i];
            if(current.m_IsValid)
            {
                current.DebugDraw(m_DebugDrawFlags);
                if(current.GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                    current.GetCollisionShape()->DebugDraw(&current);
            }
        }
    }

    // Ray-AABB intersection test
    static bool RayIntersectsAABB(const Vec3& origin, const Vec3& invDir, const Maths::BoundingBox& aabb, float& tMin, float& tMax)
    {
        Vec3 t1 = (aabb.Min() - origin) * invDir;
        Vec3 t2 = (aabb.Max() - origin) * invDir;

        Vec3 tmin = Maths::Min(t1, t2);
        Vec3 tmax = Maths::Max(t1, t2);

        tMin = Maths::Max(Maths::Max(tmin.x, tmin.y), tmin.z);
        tMax = Maths::Min(Maths::Min(tmax.x, tmax.y), tmax.z);

        return tMax >= Maths::Max(tMin, 0.0f);
    }

    // Ray-Sphere intersection
    static bool RayIntersectsSphere(const Vec3& origin, const Vec3& dir, const Vec3& center, float radius, float& t)
    {
        Vec3 oc = origin - center;
        float b = Maths::Dot(oc, dir);
        float c = Maths::Dot(oc, oc) - radius * radius;

        if(c > 0.0f && b > 0.0f)
            return false;

        float discriminant = b * b - c;
        if(discriminant < 0.0f)
            return false;

        t = -b - Maths::Sqrt(discriminant);
        if(t < 0.0f)
            t = -b + Maths::Sqrt(discriminant);

        return t >= 0.0f;
    }

    RaycastHit LumosPhysicsEngine::Raycast(const RaycastQuery& query) const
    {
        LUMOS_PROFILE_FUNCTION();
        RaycastHit result;
        float closestDist = query.MaxDistance;

        Vec3 invDir = Vec3(
            query.Direction.x != 0.0f ? 1.0f / query.Direction.x : 1e30f,
            query.Direction.y != 0.0f ? 1.0f / query.Direction.y : 1e30f,
            query.Direction.z != 0.0f ? 1.0f / query.Direction.z : 1e30f);

        for(u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& body = m_RigidBodies[i];
            if(!body.m_IsValid || !body.GetCollisionShape())
                continue;

            // Layer mask check
            if((query.LayerMask & (1 << body.GetCollisionLayer())) == 0)
                continue;

            // Skip triggers if not wanted
            if(body.GetIsTrigger() && !query.HitTriggers)
                continue;

            // AABB early-out
            float tMin, tMax;
            if(!RayIntersectsAABB(query.Origin, invDir, body.GetWorldSpaceAABB(), tMin, tMax))
                continue;

            if(tMin > closestDist)
                continue;

            // Shape-specific intersection
            auto shape = body.GetCollisionShape();
            float t    = closestDist;

            switch(shape->GetType())
            {
            case CollisionShapeType::CollisionSphere:
            {
                auto* sphere = static_cast<SphereCollisionShape*>(shape.get());
                if(RayIntersectsSphere(query.Origin, query.Direction, body.GetPosition(), sphere->GetRadius(), t))
                {
                    if(t < closestDist && t <= query.MaxDistance)
                    {
                        closestDist    = t;
                        result.Body    = &body;
                        result.Shape   = shape.get();
                        result.Distance = t;
                        result.Point   = query.Origin + query.Direction * t;
                        result.Normal  = (result.Point - body.GetPosition()).Normalised();
                    }
                }
                break;
            }
            case CollisionShapeType::CollisionCuboid:
            {
                // Use AABB result for cuboids (approximate for rotated)
                if(tMin >= 0.0f && tMin < closestDist && tMin <= query.MaxDistance)
                {
                    closestDist    = tMin;
                    result.Body    = &body;
                    result.Shape   = shape.get();
                    result.Distance = tMin;
                    result.Point   = query.Origin + query.Direction * tMin;
                    // Approximate normal from AABB
                    Vec3 center    = body.GetWorldSpaceAABB().Center();
                    Vec3 toCenter  = result.Point - center;
                    Vec3 halfSize  = body.GetWorldSpaceAABB().Size() * 0.5f;
                    Vec3 d         = toCenter / halfSize;
                    if(Maths::Abs(d.x) > Maths::Abs(d.y) && Maths::Abs(d.x) > Maths::Abs(d.z))
                        result.Normal = Vec3(d.x > 0 ? 1.0f : -1.0f, 0.0f, 0.0f);
                    else if(Maths::Abs(d.y) > Maths::Abs(d.z))
                        result.Normal = Vec3(0.0f, d.y > 0 ? 1.0f : -1.0f, 0.0f);
                    else
                        result.Normal = Vec3(0.0f, 0.0f, d.z > 0 ? 1.0f : -1.0f);
                }
                break;
            }
            case CollisionShapeType::CollisionCapsule:
            {
                auto* capsule = static_cast<CapsuleCollisionShape*>(shape.get());
                float capRadius = capsule->GetRadius();
                float halfH = capsule->GetHeight() * 0.5f;

                // Transform ray into capsule local space (Y-aligned)
                Mat4 invWS = Mat4::Inverse(body.GetWorldSpaceTransform());
                Vec3 localOrigin = invWS * Vec4(query.Origin, 1.0f);
                Vec3 localDir = (invWS * Vec4(query.Direction, 0.0f)).Normalised();

                float bestT = FLT_MAX;
                Vec3 bestLocalPoint;
                bool hitCapsule = false;

                // Ray vs infinite cylinder (radius=capRadius, Y-axis)
                float a_cyl = localDir.x * localDir.x + localDir.z * localDir.z;
                float b_cyl = 2.0f * (localOrigin.x * localDir.x + localOrigin.z * localDir.z);
                float c_cyl = localOrigin.x * localOrigin.x + localOrigin.z * localOrigin.z - capRadius * capRadius;
                float disc_cyl = b_cyl * b_cyl - 4.0f * a_cyl * c_cyl;

                if(a_cyl > Maths::M_EPSILON && disc_cyl >= 0.0f)
                {
                    float sqrtDisc = Maths::Sqrt(disc_cyl);
                    float t0 = (-b_cyl - sqrtDisc) / (2.0f * a_cyl);
                    float t1 = (-b_cyl + sqrtDisc) / (2.0f * a_cyl);

                    for(float tc : { t0, t1 })
                    {
                        if(tc < 0.0f) continue;
                        Vec3 p = localOrigin + localDir * tc;
                        if(p.y >= -halfH && p.y <= halfH && tc < bestT)
                        {
                            bestT = tc;
                            bestLocalPoint = p;
                            hitCapsule = true;
                        }
                    }
                }

                // Ray vs top hemisphere sphere
                float tSphere;
                Vec3 topCenter(0, halfH, 0);
                if(RayIntersectsSphere(localOrigin, localDir, topCenter, capRadius, tSphere) && tSphere < bestT)
                {
                    Vec3 p = localOrigin + localDir * tSphere;
                    if(p.y >= halfH)
                    {
                        bestT = tSphere;
                        bestLocalPoint = p;
                        hitCapsule = true;
                    }
                }

                // Ray vs bottom hemisphere sphere
                Vec3 bottomCenter(0, -halfH, 0);
                if(RayIntersectsSphere(localOrigin, localDir, bottomCenter, capRadius, tSphere) && tSphere < bestT)
                {
                    Vec3 p = localOrigin + localDir * tSphere;
                    if(p.y <= -halfH)
                    {
                        bestT = tSphere;
                        bestLocalPoint = p;
                        hitCapsule = true;
                    }
                }

                if(hitCapsule)
                {
                    // Convert back to world space distance
                    Vec3 worldHit = body.GetWorldSpaceTransform() * Vec4(bestLocalPoint, 1.0f);
                    float worldT = Maths::Length(worldHit - query.Origin);

                    if(worldT < closestDist && worldT <= query.MaxDistance)
                    {
                        closestDist    = worldT;
                        result.Body    = &body;
                        result.Shape   = shape.get();
                        result.Distance = worldT;
                        result.Point   = worldHit;
                        // Compute normal: project hit point onto capsule segment
                        float clampedY = Maths::Clamp(bestLocalPoint.y, -halfH, halfH);
                        Vec3 localNormal = (bestLocalPoint - Vec3(0, clampedY, 0)).Normalised();
                        result.Normal = (Mat4(body.GetOrientation()) * Vec4(localNormal, 0.0f)).Normalised();
                    }
                }
                break;
            }
            default:
                // Fall back to AABB for unknown shapes
                if(tMin >= 0.0f && tMin < closestDist && tMin <= query.MaxDistance)
                {
                    closestDist    = tMin;
                    result.Body    = &body;
                    result.Shape   = shape.get();
                    result.Distance = tMin;
                    result.Point   = query.Origin + query.Direction * tMin;
                    result.Normal  = Vec3(0.0f, 1.0f, 0.0f);
                }
                break;
            }
        }

        return result;
    }

    RaycastHit LumosPhysicsEngine::Raycast(const Vec3& origin, const Vec3& direction, float maxDistance, uint16_t layerMask) const
    {
        RaycastQuery query(origin, direction, maxDistance);
        query.LayerMask = layerMask;
        return Raycast(query);
    }

    bool LumosPhysicsEngine::RaycastAll(const RaycastQuery& query, TDArray<RaycastHit>& results, uint32_t maxResults) const
    {
        LUMOS_PROFILE_FUNCTION();

        Vec3 invDir = Vec3(
            query.Direction.x != 0.0f ? 1.0f / query.Direction.x : 1e30f,
            query.Direction.y != 0.0f ? 1.0f / query.Direction.y : 1e30f,
            query.Direction.z != 0.0f ? 1.0f / query.Direction.z : 1e30f);

        for(u32 i = 0; i < m_RigidBodyCount && results.Size() < maxResults; i++)
        {
            RigidBody3D& body = m_RigidBodies[i];
            if(!body.m_IsValid || !body.GetCollisionShape())
                continue;

            if((query.LayerMask & (1 << body.GetCollisionLayer())) == 0)
                continue;

            if(body.GetIsTrigger() && !query.HitTriggers)
                continue;

            float tMin, tMax;
            if(!RayIntersectsAABB(query.Origin, invDir, body.GetWorldSpaceAABB(), tMin, tMax))
                continue;

            if(tMin > query.MaxDistance)
                continue;

            // Simplified: add hit at AABB intersection
            RaycastHit hit;
            hit.Body     = &body;
            hit.Shape    = body.GetCollisionShape().get();
            hit.Distance = tMin >= 0.0f ? tMin : 0.0f;
            hit.Point    = query.Origin + query.Direction * hit.Distance;
            hit.Normal   = Vec3(0.0f, 1.0f, 0.0f);

            results.PushBack(hit);
        }

        // Sort by distance
        for(u32 i = 0; i < results.Size(); i++)
        {
            for(u32 j = i + 1; j < results.Size(); j++)
            {
                if(results[j].Distance < results[i].Distance)
                    Swap(results[i], results[j]);
            }
        }

        return !results.Empty();
    }

}
