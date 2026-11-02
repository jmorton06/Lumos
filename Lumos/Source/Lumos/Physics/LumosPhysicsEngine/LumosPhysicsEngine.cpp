#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "RigidBody3D.h"
#include "Narrowphase/CollisionDetection.h"
#include "Broadphase/BruteForceBroadphase.h"
#include "Broadphase/OctreeBroadphase.h"
#include "Integration.h"
#include "RaycastVehicle.h"
#include "Constraints/Constraint.h"
#include "CollisionShapes/SphereCollisionShape.h"
#include "CollisionShapes/Hull.h"
#include "CollisionShapes/MeshCollisionShape.h"
#include "CollisionShapes/HullCollisionShape.h"
#include "CollisionShapes/PyramidCollisionShape.h"
#include "CollisionShapes/CuboidCollisionShape.h"
#include "CollisionShapes/CapsuleCollisionShape.h"
#include "CollisionShapes/TerrainCollisionShape.h"
#include "Scene/Component/ModelComponent.h"
#include "Graphics/Model.h"
#include "Graphics/Terrain.h"
#include "Utilities/TimeStep.h"
#include "Utilities/Timer.h"
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

        m_FrameArena        = ArenaAlloc(Megabytes(32));
        m_Arena             = ArenaAlloc(m_MaxRigidBodyCount * sizeof(RigidBody3D) * 2 + sizeof(Mutex));
        m_RigidBodies       = PushArray(m_Arena, RigidBody3D, m_MaxRigidBodyCount);
        m_RigidBodyFreeList = TDArray<RigidBody3D*>(m_Arena);
        m_RigidBodyFreeList.Reserve(m_MaxRigidBodyCount);

        m_ManifoldLock = PushArray(m_Arena, Mutex, 1);
        MutexInit(m_ManifoldLock);
        MutexSetName(m_ManifoldLock, "Physics Manifolds");

        m_BroadphaseType      = BroadphaseType::OCTREE;
        m_BroadphaseDetection = CreateSharedPtr<OctreeBroadphase>(5, 8);
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
        for(u32 i = 0; i < m_Vehicles.Size(); i++)
            delete m_Vehicles[i];
        m_Vehicles.Clear();

        MutexDestroy(m_ManifoldLock);

        ArenaRelease(m_Arena);
        ArenaRelease(m_FrameArena);

        CollisionDetection::Release();
    }

    void LumosPhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_IsPaused)
            return;

        if(m_ManualStep)
        {
            ManualUpdate(scene);
            return;
        }

        if(!GatherConstraints(scene))
            return;

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

    bool LumosPhysicsEngine::GatherConstraints(Scene* scene)
    {
        auto& registry    = scene->GetRegistry();
        m_ConstraintCount = 0;
        ArenaClear(m_FrameArena);

        LUMOS_PROFILE_SCOPE("Physics::Get Spring Constraints");
        auto viewSpring = registry.view<SpringConstraintComponent>();
        auto viewAxis   = registry.view<AxisConstraintComponent>();
        auto viewDis    = registry.view<DistanceConstraintComponent>();
        auto viewWeld   = registry.view<WeldConstraintComponent>();

        m_ConstraintCount = (uint32_t)viewSpring.size() + (uint32_t)viewAxis.size() + (uint32_t)viewDis.size() + (uint32_t)viewWeld.size();

        if(m_ConstraintCount == 0 && m_Vehicles.Empty())
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
                return false;
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

        m_Stats.ConstraintCount = m_ConstraintCount;
        return true;
    }

    void LumosPhysicsEngine::SetManualStep(bool manual)
    {
        if(m_ManualStep == manual)
            return;

        // a half-done substep is dropped, the next one starts clean
        m_ManualStep    = manual;
        m_PendingStages = 0;
        m_UpdateAccum   = 0.0f;
        m_NextStage     = 0;
        m_LastStage     = PhysicsStage_Count;
        m_ManualScene   = nullptr;
    }

    void LumosPhysicsEngine::ManualUpdate(Scene* scene)
    {
        // contacts from another scene point at bodies that are gone
        if(scene != m_ManualScene)
        {
            m_ManualScene   = scene;
            m_NextStage     = 0;
            m_LastStage     = PhysicsStage_Count;
            m_ManifoldCount = 0;
        }

        while(m_PendingStages > 0)
        {
            m_PendingStages--;

            // The frame arena holds the manifolds, so it is only reset between substeps.
            if(m_NextStage == 0)
            {
                if(!GatherConstraints(scene))
                {
                    m_PendingStages = 0;
                    return;
                }
                m_CollisionEvents.Clear();
                m_CurrCollisionPairs.Clear();
            }

            RunStage((PhysicsStage)m_NextStage);
            m_LastStage = m_NextStage;

            if(++m_NextStage == PhysicsStage_Count)
            {
                m_NextStage = 0;
                DispatchCollisionCallbacks(scene);
                Swap(m_PrevCollisionPairs, m_CurrCollisionPairs);
            }
        }
    }

    void LumosPhysicsEngine::UpdatePhysics()
    {
        for(uint32_t stage = 0; stage < PhysicsStage_Count; stage++)
            RunStage((PhysicsStage)stage);
    }

    void LumosPhysicsEngine::RunStage(PhysicsStage stage)
    {
        const TimeStamp start = Timer::Now();

        switch(stage)
        {
        case PhysicsStage_Broadphase:
        {
            m_ManifoldCount = 0;
            m_MaxManifolds  = 1000;
            m_Manifolds     = PushArrayNoZero(m_FrameArena, Manifold, m_MaxManifolds);

            for(u32 i = 0; i < m_RigidBodyCount; i++)
            {
                RigidBody3D& body = m_RigidBodies[i];
                if(body.m_IsValid)
                    body.SnapshotPrev();
            }

            BroadPhaseCollisions();
            break;
        }
        case PhysicsStage_Narrowphase:
        {
            if(m_ParallelNarrowphase && m_BroadphaseCollisionPairs.Size() > 32)
                NarrowPhaseCollisionsParallel();
            else
                NarrowPhaseCollisions();
            break;
        }
        case PhysicsStage_Forces:
        {
            for(u32 i = 0; i < m_RigidBodyCount; i++)
            {
                RigidBody3D& body = m_RigidBodies[i];
                if(!body.m_IsValid || body.m_Static || !body.IsAwake())
                    continue;
                if(body.m_InvMass > 0.0f)
                    body.m_LinearVelocity += m_Gravity * s_UpdateTimestep;
            }

            for(u32 i = 0; i < m_Vehicles.Size(); i++)
                m_Vehicles[i]->Update(this, s_UpdateTimestep);
            break;
        }
        case PhysicsStage_Solve:
            SolveConstraints();
            break;
        case PhysicsStage_Integrate:
        {
            for(uint32_t i = 0; i < m_PositionIterations; i++)
                UpdateRigidBodies();
            break;
        }
        case PhysicsStage_Rest:
        {
            for(u32 i = 0; i < m_RigidBodyCount; i++)
            {
                RigidBody3D& current = m_RigidBodies[i];
                if(current.m_IsValid)
                    current.RestTest();
            }
            break;
        }
        default:
            break;
        }

        m_StageMs[stage] = Timer::Duration(start, Timer::Now(), 1000.0f);
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

    // Continuous collision. Collisions are normally found by looking for overlap AFTER
    // each move, so a body that travels further than its own thickness in one step can
    // jump clean over something thin and never overlap it.
    //
    // For bodies moving that fast - their tips included, so a spinning plank counts - the
    // step is replayed in pieces no longer than the body is thick, running the real
    // shape-against-shape test at each one. The move stops at the first piece that
    // touches something; the two then overlap slightly and the ordinary contact solve
    // takes it from there. Only things near the path are tested.
    void LumosPhysicsEngine::StopAtFirstContact(RigidBody3D* obj, const Vec3& startPosition, const Quat& startOrientation, float dt) const
    {
        const auto& shape = obj->GetCollisionShape();
        if(!shape)
            return;

        // thinnest half-size, and how far the shape reaches from its centre
        float inner = shape->GetSize();
        float reach = inner;
        switch(shape->GetType())
        {
        case CollisionShapeType::CollisionCuboid:
        {
            auto* cuboid    = static_cast<CuboidCollisionShape*>(shape.get());
            const Vec3 half = Vec3(cuboid->GetHalfWidth(), cuboid->GetHalfHeight(), cuboid->GetHalfDepth());
            inner           = Maths::Min(half.x, Maths::Min(half.y, half.z));
            reach           = Maths::Length(half);
            break;
        }
        case CollisionShapeType::CollisionCapsule:
        {
            auto* capsule = static_cast<CapsuleCollisionShape*>(shape.get());
            inner         = capsule->GetRadius();
            reach         = capsule->GetHeight() * 0.5f + inner;
            break;
        }
        default:
            break;
        }
        if(inner <= 0.0f)
            return;

        // how far the fastest-moving point of the body can have gone this step
        const float travel = Maths::Length(obj->m_LinearVelocity) * dt + Maths::Length(obj->m_AngularVelocity) * dt * reach;
        // a step no longer than the body is thick always ends overlapping whatever was in the way
        if(travel <= inner)
            return;

        const Vec3 endPosition    = obj->m_Position;
        const Quat endOrientation = obj->m_Orientation;

        // everything whose bounds come near the path
        static constexpr u32 kMaxCandidates = 32;
        RigidBody3D* candidates[kMaxCandidates];
        u32 candidateCount = 0;

        const Vec3 margin(reach + inner);
        const Vec3 pathMin = Vec3(Maths::Min(startPosition.x, endPosition.x), Maths::Min(startPosition.y, endPosition.y), Maths::Min(startPosition.z, endPosition.z)) - margin;
        const Vec3 pathMax = Vec3(Maths::Max(startPosition.x, endPosition.x), Maths::Max(startPosition.y, endPosition.y), Maths::Max(startPosition.z, endPosition.z)) + margin;

        for(u32 i = 0; i < m_RigidBodyCount && candidateCount < kMaxCandidates; i++)
        {
            RigidBody3D& other = m_RigidBodies[i];
            if(&other == obj || !other.m_IsValid || !other.GetCollisionShape() || other.GetIsTrigger())
                continue;
            if(!obj->CanCollideWith(&other))
                continue;

            const Maths::BoundingBox& bounds = other.GetWorldSpaceAABB();
            const Vec3 lo                    = bounds.Min();
            const Vec3 hi                    = bounds.Max();
            if(lo.x > pathMax.x || hi.x < pathMin.x || lo.y > pathMax.y || hi.y < pathMin.y || lo.z > pathMax.z || hi.z < pathMin.z)
                continue;

            candidates[candidateCount++] = &other;
        }
        if(candidateCount == 0)
            return;

        auto placeAt = [&](float fraction)
        {
            obj->m_Position    = Vec3::Lerp(startPosition, endPosition, fraction);
            obj->m_Orientation = Quat::Slerp(startOrientation, endOrientation, fraction);
            obj->m_Orientation.Normalise();
            obj->m_WSTransformInvalidated = true;
            obj->m_WSAabbInvalidated      = true;
        };
        auto touching = [&](RigidBody3D* other)
        {
            return CollisionDetection::Get().CheckCollision(obj, other, shape.get(), other->GetCollisionShape().get(), nullptr);
        };

        static constexpr int kMaxPieces = 32;
        const int pieces                = Maths::Clamp((int)(travel / inner) + 1, 2, kMaxPieces);

        // Things it is already touching at the start. Resting on or sliding along one is the
        // contact solver's business and must not stop the move - but pushing further INTO
        // one must, or a body stopped at a wall by one step would be let through by the next.
        float startDepth[kMaxCandidates];
        bool startTouching[kMaxCandidates];
        bool anyTouching = false;
        placeAt(0.0f);
        for(u32 i = 0; i < candidateCount; i++)
        {
            CollisionData data;
            startTouching[i] = CollisionDetection::Get().CheckCollision(obj, candidates[i], shape.get(), candidates[i]->GetCollisionShape().get(), &data);
            startDepth[i]    = startTouching[i] ? Maths::Abs(data.penetration) : 0.0f;
            anyTouching      = anyTouching || startTouching[i];
        }
        if(anyTouching)
        {
            placeAt(1.0f / (float)pieces);
            for(u32 i = 0; i < candidateCount; i++)
            {
                if(!startTouching[i])
                    continue;
                CollisionData data;
                const bool still = CollisionDetection::Get().CheckCollision(obj, candidates[i], shape.get(), candidates[i]->GetCollisionShape().get(), &data);
                if(still && Maths::Abs(data.penetration) > startDepth[i] + inner * 0.05f)
                {
                    placeAt(0.0f); // going deeper: stay put this step
                    return;
                }
            }
            // the rest are being rested on or slid along: leave them out of the sweep
            for(u32 i = 0; i < candidateCount;)
            {
                if(startTouching[i])
                {
                    --candidateCount;
                    candidates[i]    = candidates[candidateCount];
                    startTouching[i] = startTouching[candidateCount];
                }
                else
                    i++;
            }
        }

        if(candidateCount > 0)
        {
            for(int piece = 1; piece < pieces; piece++)
            {
                placeAt((float)piece / (float)pieces);
                for(u32 i = 0; i < candidateCount; i++)
                {
                    if(touching(candidates[i]))
                        return; // leave the body here, just touching
                }
            }
        }

        placeAt(1.0f);
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

    RaycastVehicle* LumosPhysicsEngine::CreateVehicle(RigidBody3D* chassis)
    {
        RaycastVehicle* vehicle = new RaycastVehicle();
        vehicle->Init(chassis);
        m_Vehicles.PushBack(vehicle);
        return vehicle;
    }

    void LumosPhysicsEngine::DestroyVehicle(RaycastVehicle* vehicle)
    {
        for(u32 i = 0; i < m_Vehicles.Size(); i++)
        {
            if(m_Vehicles[i] == vehicle)
            {
                m_Vehicles[i] = m_Vehicles.Back();
                m_Vehicles.PopBack();
                delete vehicle;
                return;
            }
        }
    }

    void LumosPhysicsEngine::SyncTransforms(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();

        if(!scene)
            return;

        const float alpha = (s_UpdateTimestep > 0.0f)
            ? Maths::Clamp(m_UpdateAccum / s_UpdateTimestep, 0.0f, 1.0f)
            : 0.0f;
        // stepping by hand shows the real body state, not a blend towards it
        const bool interp = !m_ManualStep && alpha > Maths::M_EPSILON && alpha < 1.0f - Maths::M_EPSILON;

        auto& registry = scene->GetRegistry();
        auto group     = registry.group<RigidBody3DComponent>(entt::get<Maths::Transform>);

        for(auto entity : group)
        {
            const auto& [phys, trans] = group.get<RigidBody3DComponent, Maths::Transform>(entity);

            auto* body = phys.GetRigidBody();
            if(!body->GetIsStatic() && body->IsAwake())
            {
                ASSERT(body->GetPosition().IsValid());
                ASSERT(body->GetOrientation().IsValid());

                if(interp)
                {
                    trans.SetLocalPosition(Vec3::Lerp(body->GetPrevPosition(), body->GetPosition(), alpha));
                    trans.SetLocalOrientation(Quat::Slerp(body->GetPrevOrientation(), body->GetOrientation(), alpha));
                }
                else
                {
                    trans.SetLocalPosition(body->GetPosition());
                    trans.SetLocalOrientation(body->GetOrientation());
                }
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

        // Fill empty mesh collision shapes from the entity's model once it has loaded
        {
            auto meshView = registry.view<Graphics::ModelComponent, RigidBody3DComponent, Maths::Transform>();
            for(auto entity : meshView)
            {
                auto* body = meshView.get<RigidBody3DComponent>(entity).GetRigidBody();
                if(!body)
                    continue;
                auto shape = body->GetCollisionShape();
                if(!shape || shape->GetType() != CollisionShapeType::CollisionMesh)
                    continue;
                auto* meshShape = static_cast<MeshCollisionShape*>(shape.get());
                if(meshShape->IsBuilt())
                    continue;

                // the body carries position + rotation; the entity's scale is baked into the triangles
                auto& trans     = meshView.get<Maths::Transform>(entity);
                auto placeBody = [&]()
                {
                    body->SetIsStatic(true);
                    body->SetPosition(trans.GetWorldPosition());
                    body->SetOrientation(trans.GetWorldOrientation());
                    body->SetCollisionShape(shape); // resize the body's bounds
                };

                // a baked .lcol needs neither the model nor a BVH build
                const auto start = Timer::Now();
                if(meshShape->ShouldLoadFile())
                {
                    meshShape->SetShouldLoadFile(false); // one try: a bad or missing file is rebuilt below
                    if(meshShape->LoadFromFile(Str8C((char*)meshShape->GetFilePath())))
                    {
                        placeBody();
                        LINFO("Mesh collision loaded: %s, %u triangles in %.1f ms", meshShape->GetFilePath(), meshShape->GetTriangleCount(), Timer::Duration(start, Timer::Now(), 1000.0f));
                        continue;
                    }
                }

                auto& model = meshView.get<Graphics::ModelComponent>(entity);
                if(!model.ModelRef || model.ModelRef->IsLoading() || model.ModelRef->GetMeshes().Empty())
                    continue;

                const Mat4 bodyToWorld = Mat4::Translation(trans.GetWorldPosition()) * trans.GetWorldOrientation().ToMatrix4();
                if(meshShape->BuildFromModel(model.ModelRef.get(), Mat4::Inverse(bodyToWorld) * trans.GetWorldMatrix()))
                {
                    placeBody();
                    LINFO("Mesh collision built: %u triangles in %.1f ms", meshShape->GetTriangleCount(), Timer::Duration(start, Timer::Now(), 1000.0f));
                    if(meshShape->GetFilePath()[0] != 0)
                    {
                        if(meshShape->SaveToFile(Str8C((char*)meshShape->GetFilePath())))
                            LINFO("Mesh collision saved: %s", meshShape->GetFilePath());
                        else
                            LWARN("Mesh collision could not be saved: %s", meshShape->GetFilePath());
                    }
                }
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
            const float damping         = m_DampingFactor;
            const Vec3 startPosition    = obj->m_Position;
            const Quat startOrientation = obj->m_Orientation;

            // Gravity is applied once per frame in UpdatePhysics before SolveConstraints.

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

            if(m_ContinuousCollision)
                StopAtFirstContact(obj, startPosition, startOrientation, dt);

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
                    DebugRenderer::DrawThickLine(cp.pObjectA->GetPosition(), cp.pObjectB->GetPosition(), DEBUG_LINE_WIDTH, false, colour);
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
                                DebugRenderer::DrawThickLine(colData.pointOnPlane, colData.pointOnPlane - colData.normal * colData.penetration, DEBUG_LINE_WIDTH * 1.25f, false, Vec4(0.0f, 0.0f, 1.0f, 1.0f), 3.0f);
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

        const uint32_t pairCount = (uint32_t)m_BroadphaseCollisionPairs.Size();
        const uint32_t groupSize = 16; // Process 16 pairs per job

        m_Stats.NarrowPhaseCount = pairCount;
        std::atomic<uint32_t> collisionCount { 0 };

        const uint32_t eventBase = (uint32_t)m_CollisionEvents.Size();
        m_CollisionEvents.Resize(eventBase + pairCount);
        m_CurrCollisionPairs.Resize(eventBase + pairCount);
        std::atomic<uint32_t> eventIdx { eventBase };
        std::atomic<uint32_t> manifoldIdx { m_ManifoldCount };

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

            // Queue collision event into our own preallocated slot (lock-free).
            CollisionEvent3D evt;
            evt.BodyA = cp.pObjectA;
            evt.BodyB = cp.pObjectB;
            evt.InfoA = { cp.pObjectB, colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectB->GetIsTrigger() };
            evt.InfoB = { cp.pObjectA, -colData.normal, colData.pointOnPlane, colData.penetration, cp.pObjectA->GetIsTrigger() };
            const uint32_t ei      = eventIdx.fetch_add(1, std::memory_order_relaxed);
            m_CollisionEvents[ei]  = evt;
            m_CurrCollisionPairs[ei] = { cp.pObjectA, cp.pObjectB };

            // Collision callbacks (may not be thread-safe, but user's responsibility)
            const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
            const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

            if(!okA || !okB)
                return;

            if(cp.pObjectA->GetIsTrigger() || cp.pObjectB->GetIsTrigger())
                return;

            Manifold local;
            local.Initiate(cp.pObjectA, cp.pObjectB, m_BaumgarteScalar, m_BaumgarteSlop);
            if(CollisionDetection::Get().BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, &local))
            {
                const uint32_t mi = manifoldIdx.fetch_add(1, std::memory_order_relaxed);
                if(mi < m_MaxManifolds)
                {
                    m_Manifolds[mi] = local;
                    cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, &m_Manifolds[mi]);
                    cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, &m_Manifolds[mi]);
                    collisionCount.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        System::JobSystem::Wait(ctx);

        // Trim to what was actually written.
        m_CollisionEvents.Resize(eventIdx.load());
        m_CurrCollisionPairs.Resize(eventIdx.load());
        m_ManifoldCount        = Maths::Min(manifoldIdx.load(), m_MaxManifolds);
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

        // Warm-start: apply previous frame's impulses before iterating.
        if(m_WarmStartingEnabled)
            WarmStartManifolds();

        {
            LUMOS_PROFILE_SCOPE("Apply Impulses");
            ApplyImpulses();
        }

        if(m_WarmStartingEnabled)
            SavePersistentImpulses();
    }

    void LumosPhysicsEngine::ApplyImpulses()
    {
        const u32 manifoldCount   = m_ManifoldCount;
        const u32 constraintCount = m_ConstraintCount;
        const u32 velIters        = m_VelocityIterations;
        const u32 bodyCount       = m_RigidBodyCount;

        const bool useParallel = m_ParallelSolver
            && System::JobSystem::GetThreadCount() > 1
            && bodyCount > 0
            && (manifoldCount + constraintCount) >= 64;

        if(!useParallel)
        {
            for(u32 i = 0; i < velIters; i++)
            {
                for(u32 index = 0; index < manifoldCount; index++)
                    m_Manifolds[index].ApplyImpulse();
                for(u32 index = 0; index < constraintCount; index++)
                    m_Constraints[index]->ApplyImpulse();
            }
            return;
        }

        RigidBody3D* pool = m_RigidBodies;

        u32* parent = PushArrayNoZero(m_FrameArena, u32, bodyCount);
        for(u32 i = 0; i < bodyCount; i++)
            parent[i] = i;

        auto dynIndex = [pool, bodyCount](RigidBody3D* b) -> u32
        {
            if(!b || b->GetInverseMass() <= 0.0f || b < pool || b >= pool + bodyCount)
                return UINT32_MAX;
            return (u32)(b - pool);
        };
        auto findRoot = [&](u32 x) -> u32
        {
            while(parent[x] != x)
            {
                parent[x] = parent[parent[x]];
                x         = parent[x];
            }
            return x;
        };
        auto unite = [&](u32 a, u32 b)
        {
            u32 ra = findRoot(a), rb = findRoot(b);
            if(ra != rb)
                parent[ra] = rb;
        };

        for(u32 i = 0; i < manifoldCount; i++)
        {
            u32 ia = dynIndex(m_Manifolds[i].NodeA());
            u32 ib = dynIndex(m_Manifolds[i].NodeB());
            if(ia != UINT32_MAX && ib != UINT32_MAX)
                unite(ia, ib);
        }
        for(u32 i = 0; i < constraintCount; i++)
        {
            RigidBody3D* a = nullptr;
            RigidBody3D* b = nullptr;
            m_Constraints[i]->GetBodies(a, b);
            u32 ia = dynIndex(a);
            u32 ib = dynIndex(b);
            if(ia != UINT32_MAX && ib != UINT32_MAX)
                unite(ia, ib);
        }

        u32* headM = PushArrayNoZero(m_FrameArena, u32, bodyCount);
        u32* headC = PushArrayNoZero(m_FrameArena, u32, bodyCount);
        for(u32 i = 0; i < bodyCount; i++)
        {
            headM[i] = UINT32_MAX;
            headC[i] = UINT32_MAX;
        }
        u32* nextM = manifoldCount ? PushArrayNoZero(m_FrameArena, u32, manifoldCount) : nullptr;
        u32* nextC = constraintCount ? PushArrayNoZero(m_FrameArena, u32, constraintCount) : nullptr;
        u8* seen   = PushArray(m_FrameArena, u8, bodyCount); // zeroed

        auto islandOf = [&](RigidBody3D* a, RigidBody3D* b) -> u32
        {
            u32 ia = dynIndex(a);
            if(ia != UINT32_MAX)
                return findRoot(ia);
            u32 ib = dynIndex(b);
            if(ib != UINT32_MAX)
                return findRoot(ib);
            return UINT32_MAX; // both static — broadphase shouldn't produce these
        };

        TDArray<u32> islands(m_FrameArena);
        islands.Reserve(64);

        for(u32 i = 0; i < manifoldCount; i++)
        {
            u32 r = islandOf(m_Manifolds[i].NodeA(), m_Manifolds[i].NodeB());
            if(r == UINT32_MAX)
                continue;
            nextM[i] = headM[r];
            headM[r] = i;
            if(!seen[r])
            {
                seen[r] = 1;
                islands.PushBack(r);
            }
        }
        for(u32 i = 0; i < constraintCount; i++)
        {
            RigidBody3D* a = nullptr;
            RigidBody3D* b = nullptr;
            m_Constraints[i]->GetBodies(a, b);
            u32 r = islandOf(a, b);
            if(r == UINT32_MAX)
                continue;
            nextC[i] = headC[r];
            headC[r] = i;
            if(!seen[r])
            {
                seen[r] = 1;
                islands.PushBack(r);
            }
        }

        const u32 islandCount              = (u32)islands.Size();
        const u32* islandRoots             = islands.Data();
        Manifold* manifolds                = m_Manifolds;
        SharedPtr<Constraint>* constraints = m_Constraints;

        System::JobSystem::Context ctx;
        System::JobSystem::Dispatch(ctx, islandCount, 1,
                                    [islandRoots, headM, headC, nextM, nextC, manifolds, constraints, velIters](JobDispatchArgs args)
                                    {
                                        const u32 r = islandRoots[args.jobIndex];
                                        for(u32 it = 0; it < velIters; it++)
                                        {
                                            for(u32 mi = headM[r]; mi != UINT32_MAX; mi = nextM[mi])
                                                manifolds[mi].ApplyImpulse();
                                            for(u32 ci = headC[r]; ci != UINT32_MAX; ci = nextC[ci])
                                                constraints[ci]->ApplyImpulse();
                                        }
                                    });
        System::JobSystem::Wait(ctx);
    }

    namespace
    {
        static uint64_t MakeBodyPairKey(const RigidBody3D* a, const RigidBody3D* b)
        {
            uintptr_t pa = reinterpret_cast<uintptr_t>(a);
            uintptr_t pb = reinterpret_cast<uintptr_t>(b);
            if(pa > pb)
            {
                uintptr_t tmp = pa;
                pa            = pb;
                pb            = tmp;
            }
            uint64_t key = static_cast<uint64_t>(pa);
            key ^= static_cast<uint64_t>(pb) + 0x9e3779b97f4a7c15ULL + (key << 12) + (key >> 4);
            return key;
        }
    }

    void LumosPhysicsEngine::PersistentTable::Reset(uint32_t expected)
    {
        uint32_t capacity = 64;
        while(capacity < expected * 2)
            capacity <<= 1;
        if(Keys.Size() < capacity)
        {
            Keys.Resize(capacity);
            Values.Resize(capacity);
        }
        memset(Keys.Data(), 0, Keys.Size() * sizeof(uint64_t));
        Count = 0;
    }

    LumosPhysicsEngine::PersistentManifoldCache* LumosPhysicsEngine::PersistentTable::Find(uint64_t key)
    {
        if(Count == 0)
            return nullptr;
        key              = key ? key : 1;
        const size_t mask = Keys.Size() - 1;
        for(size_t i = key & mask;; i = (i + 1) & mask)
        {
            if(Keys[i] == key)
                return &Values[i];
            if(Keys[i] == 0)
                return nullptr;
        }
    }

    LumosPhysicsEngine::PersistentManifoldCache* LumosPhysicsEngine::PersistentTable::Insert(uint64_t key)
    {
        key              = key ? key : 1;
        const size_t mask = Keys.Size() - 1;
        for(size_t i = key & mask;; i = (i + 1) & mask)
        {
            if(Keys[i] == key)
                return &Values[i];
            if(Keys[i] == 0)
            {
                Keys[i] = key;
                Count++;
                return &Values[i];
            }
        }
    }

    void LumosPhysicsEngine::WarmStartManifolds()
    {
        LUMOS_PROFILE_FUNCTION();

        PersistentTable& table = m_PersistentTables[m_PersistentCurrent];

        const float kMatchToleranceSq = 0.02f * 0.02f;

        for(uint32_t i = 0; i < m_ManifoldCount; i++)
        {
            Manifold& m   = m_Manifolds[i];
            RigidBody3D* a = m.NodeA();
            RigidBody3D* b = m.NodeB();
            if(!a || !b)
                continue;

            PersistentManifoldCache* found = table.Find(MakeBodyPairKey(a, b));
            if(!found)
                continue;
            PersistentManifoldCache& cache = *found;

            if(cache.savedNodeA != a)
                continue;

            ContactPoint* contacts  = m.GetContacts();
            const uint32_t contactN = m.GetContactCount();

            const Quat invOrientA = a->GetOrientation().Inverse();
            const Quat invOrientB = b->GetOrientation().Inverse();

            for(uint32_t ci = 0; ci < contactN; ci++)
            {
                ContactPoint& c = contacts[ci];

                const Vec3 currentLocalA = invOrientA * c.relPosA;
                const Vec3 currentLocalB = invOrientB * c.relPosB;

                int bestIdx     = -1;
                float bestDistSq = kMatchToleranceSq;
                for(int pi = 0; pi < cache.count; pi++)
                {
                    Vec3 dA      = currentLocalA - cache.contacts[pi].localPosA;
                    Vec3 dB      = currentLocalB - cache.contacts[pi].localPosB;
                    float distSq = Maths::Length2(dA) + Maths::Length2(dB);
                    if(distSq < bestDistSq)
                    {
                        bestDistSq = distSq;
                        bestIdx    = pi;
                    }
                }

                if(bestIdx < 0)
                    continue;

                const PersistentContactPoint& pc = cache.contacts[bestIdx];

                float jnScalar = Maths::Min(Maths::Dot(pc.normalImpulse, c.collisionNormal), 0.0f);
                float jt1      = Maths::Dot(pc.frictionImpulse, c.frictionTangent1);
                float jt2      = Maths::Dot(pc.frictionImpulse, c.frictionTangent2);
                float jr1      = Maths::Dot(pc.rollingImpulse, c.frictionTangent1);
                float jr2      = Maths::Dot(pc.rollingImpulse, c.frictionTangent2);

                c.sumImpulseContact   = jnScalar;
                c.sumImpulseFriction1 = jt1;
                c.sumImpulseFriction2 = jt2;
                c.sumImpulseRolling1  = jr1;
                c.sumImpulseRolling2  = jr2;

                Vec3 worldImpulse = c.collisionNormal * jnScalar
                    + c.frictionTangent1 * jt1
                    + c.frictionTangent2 * jt2;

                a->SetLinearVelocity(a->GetLinearVelocity()
                                     + worldImpulse * a->GetInverseMass() * a->GetLinearFactor());
                a->SetAngularVelocity(a->GetAngularVelocity()
                                      + a->GetInverseInertia() * Maths::Cross(c.relPosA, worldImpulse) * a->GetAngularFactor());
                b->SetLinearVelocity(b->GetLinearVelocity()
                                     - worldImpulse * b->GetInverseMass() * b->GetLinearFactor());
                b->SetAngularVelocity(b->GetAngularVelocity()
                                      - b->GetInverseInertia() * Maths::Cross(c.relPosB, worldImpulse) * b->GetAngularFactor());

                Vec3 worldRolling = c.frictionTangent1 * jr1 + c.frictionTangent2 * jr2;
                a->SetAngularVelocity(a->GetAngularVelocity()
                                      + a->GetInverseInertia() * worldRolling * a->GetAngularFactor());
                b->SetAngularVelocity(b->GetAngularVelocity()
                                      - b->GetInverseInertia() * worldRolling * b->GetAngularFactor());
            }
        }
    }

    void LumosPhysicsEngine::SavePersistentImpulses()
    {
        LUMOS_PROFILE_FUNCTION();

        // pairs without a manifold this substep are simply not carried over
        m_PersistentCurrent ^= 1;
        PersistentTable& table = m_PersistentTables[m_PersistentCurrent];
        table.Reset(m_ManifoldCount);

        for(uint32_t i = 0; i < m_ManifoldCount; i++)
        {
            Manifold& m   = m_Manifolds[i];
            RigidBody3D* a = m.NodeA();
            RigidBody3D* b = m.NodeB();
            if(!a || !b)
                continue;

            PersistentManifoldCache& cache = *table.Insert(MakeBodyPairKey(a, b));
            cache.savedNodeA               = a;

            const ContactPoint* contacts = m.GetContacts();
            const uint32_t contactN      = m.GetContactCount();

            const Quat invOrientA = a->GetOrientation().Inverse();
            const Quat invOrientB = b->GetOrientation().Inverse();

            const int maxStore = PersistentManifoldCache::kMaxPersistedContacts;
            cache.count        = static_cast<int>(contactN < (uint32_t)maxStore ? contactN : (uint32_t)maxStore);

            for(int ci = 0; ci < cache.count; ci++)
            {
                const ContactPoint& c    = contacts[ci];
                PersistentContactPoint& pc = cache.contacts[ci];

                pc.localPosA       = invOrientA * c.relPosA;
                pc.localPosB       = invOrientB * c.relPosB;
                pc.normalImpulse   = c.collisionNormal * c.sumImpulseContact;
                pc.frictionImpulse = c.frictionTangent1 * c.sumImpulseFriction1
                    + c.frictionTangent2 * c.sumImpulseFriction2;
                pc.rollingImpulse  = c.frictionTangent1 * c.sumImpulseRolling1
                    + c.frictionTangent2 * c.sumImpulseRolling2;
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

    static bool RayIntersectsAABB(const Vec3& origin, const Vec3& invDir, const Maths::BoundingBox& aabb, float& tMin, float& tMax)
    {
        const Vec3 lo = aabb.Min();
        const Vec3 hi = aabb.Max();

        const float tx1 = (lo.x - origin.x) * invDir.x;
        const float tx2 = (hi.x - origin.x) * invDir.x;
        const float ty1 = (lo.y - origin.y) * invDir.y;
        const float ty2 = (hi.y - origin.y) * invDir.y;
        const float tz1 = (lo.z - origin.z) * invDir.z;
        const float tz2 = (hi.z - origin.z) * invDir.z;

        tMin = Maths::Max(Maths::Max(Maths::Min(tx1, tx2), Maths::Min(ty1, ty2)), Maths::Min(tz1, tz2));
        tMax = Maths::Min(Maths::Min(Maths::Max(tx1, tx2), Maths::Max(ty1, ty2)), Maths::Max(tz1, tz2));

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

    // Ray against a capsule given by its two end points. dir must be unit length.
    // Returns the distance to the first surface hit; a ray starting inside misses.
    static bool RayIntersectsSegmentCapsule(const Vec3& origin, const Vec3& dir, const Vec3& a, const Vec3& b, float radius, float& t)
    {
        const Vec3 ba    = b - a;
        const Vec3 oa    = origin - a;
        const float baba = Maths::Dot(ba, ba);
        const float bard = Maths::Dot(ba, dir);
        const float baoa = Maths::Dot(ba, oa);
        const float rdoa = Maths::Dot(dir, oa);
        const float oaoa = Maths::Dot(oa, oa);

        // the cylinder between the ends
        const float qa = baba - bard * bard;
        const float qb = baba * rdoa - baoa * bard;
        const float qc = baba * oaoa - baoa * baoa - radius * radius * baba;
        const float h  = qb * qb - qa * qc;
        if(h < 0.0f)
            return false;

        if(qa > 1e-8f)
        {
            const float tc = (-qb - Maths::Sqrt(h)) / qa;
            const float y  = baoa + tc * bard;
            if(y > 0.0f && y < baba)
            {
                t = tc;
                return tc >= 0.0f;
            }
            // off one end: the rounded cap there
            const Vec3 oc    = y <= 0.0f ? oa : origin - b;
            const float cb   = Maths::Dot(dir, oc);
            const float cc   = Maths::Dot(oc, oc) - radius * radius;
            const float capH = cb * cb - cc;
            if(capH > 0.0f)
            {
                t = -cb - Maths::Sqrt(capH);
                return t >= 0.0f;
            }
            return false;
        }

        // ray runs along the capsule: only the caps can be hit
        bool hit = false;
        for(const Vec3& end : { a, b })
        {
            float ts;
            if(RayIntersectsSphere(origin, dir, end, radius, ts) && (!hit || ts < t))
            {
                t   = ts;
                hit = true;
            }
        }
        return hit;
    }

    // Exact test of one body against a ray (or a swept sphere when query.Radius > 0).
    // Only hits closer than maxDistance count.
    bool LumosPhysicsEngine::RaycastBody(RigidBody3D& body, const RaycastQuery& query, const Vec3& invDir, float maxDistance, RaycastHit& result) const
    {
        result            = RaycastHit();
        float closestDist = maxDistance;
        // A sphere cast is a ray against every shape grown by the sphere's radius.
        const float castRadius = Maths::Max(query.Radius, 0.0f);

        if(!body.m_IsValid || !body.GetCollisionShape())
            return false;

        if(&body == query.IgnoreBody)
            return false;

        // Layer mask check
        if((query.LayerMask & (1 << body.GetCollisionLayer())) == 0)
            return false;

        // Skip triggers if not wanted
        if(body.GetIsTrigger() && !query.HitTriggers)
            return false;

        // AABB early-out (grown by the radius for a sphere cast)
        float tMin, tMax;
        Maths::BoundingBox bounds = body.GetWorldSpaceAABB();
        if(castRadius > 0.0f)
            bounds = Maths::BoundingBox(bounds.Min() - Vec3(castRadius), bounds.Max() + Vec3(castRadius));
        if(!RayIntersectsAABB(query.Origin, invDir, bounds, tMin, tMax))
            return false;

        if(tMin > closestDist)
            return false;

        // Shape-specific intersection
        auto shape = body.GetCollisionShape();
        float t    = closestDist;

        switch(shape->GetType())
        {
        case CollisionShapeType::CollisionSphere:
        {
            auto* sphere = static_cast<SphereCollisionShape*>(shape.get());
            if(RayIntersectsSphere(query.Origin, query.Direction, body.GetPosition(), sphere->GetRadius() + castRadius, t))
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
            // Slab test in the box's own space, so rotated boxes (ramps) are exact.
            auto* cuboid       = static_cast<CuboidCollisionShape*>(shape.get());
            const Quat invRot  = body.GetOrientation().Inverse();
            const Vec3 localO  = invRot * (query.Origin - body.GetPosition());
            const Vec3 localD  = invRot * query.Direction;
            const Vec3 half    = Vec3(cuboid->GetHalfWidth(), cuboid->GetHalfHeight(), cuboid->GetHalfDepth()) + Vec3(castRadius);
            const float o[3]   = { localO.x, localO.y, localO.z };
            const float d[3]   = { localD.x, localD.y, localD.z };
            const float h[3]   = { half.x, half.y, half.z };

            float tNear = -FLT_MAX, tFar = FLT_MAX;
            int nearAxis   = 0;
            float nearSign = 1.0f;
            bool miss      = false;
            for(int axis = 0; axis < 3 && !miss; axis++)
            {
                if(Maths::Abs(d[axis]) < 1e-8f)
                {
                    miss = Maths::Abs(o[axis]) > h[axis]; // parallel to this slab and outside it
                    continue;
                }
                float t0   = (-h[axis] - o[axis]) / d[axis];
                float t1   = (h[axis] - o[axis]) / d[axis];
                float sign = -1.0f; // entering through the negative face
                if(t0 > t1)
                {
                    float tmp = t0;
                    t0        = t1;
                    t1        = tmp;
                    sign      = 1.0f;
                }
                if(t0 > tNear)
                {
                    tNear    = t0;
                    nearAxis = axis;
                    nearSign = sign;
                }
                tFar = Maths::Min(tFar, t1);
                miss = tNear > tFar;
            }

            Vec3 localNormal(0.0f);
            if(nearAxis == 0)
                localNormal.x = nearSign;
            else if(nearAxis == 1)
                localNormal.y = nearSign;
            else
                localNormal.z = nearSign;

            // A box grown by a sphere has ROUNDED edges and corners; the slab test above
            // treated them as square. If the entry point is beyond the real box on more than
            // one axis it is in an edge or corner region, and the true surface there is a
            // capsule along that edge (or the three edges meeting at that corner).
            if(castRadius > 0.0f && !miss && tNear >= 0.0f)
            {
                const Vec3 entry   = localO + localD * tNear;
                const float e[3]   = { entry.x, entry.y, entry.z };
                const float box[3] = { h[0] - castRadius, h[1] - castRadius, h[2] - castRadius };
                float corner[3];
                int outside = 0;
                bool beyond[3];
                for(int axis = 0; axis < 3; axis++)
                {
                    beyond[axis] = Maths::Abs(e[axis]) > box[axis];
                    corner[axis] = e[axis] < 0.0f ? -box[axis] : box[axis];
                    outside += beyond[axis] ? 1 : 0;
                }

                if(outside >= 2)
                {
                    miss        = true;
                    float bestT = FLT_MAX;
                    for(int axis = 0; axis < 3; axis++)
                    {
                        // the three edges that meet at the corner nearest the entry point
                        float endA[3] = { corner[0], corner[1], corner[2] };
                        float endB[3] = { corner[0], corner[1], corner[2] };
                        endA[axis]    = -box[axis];
                        endB[axis]    = box[axis];
                        const Vec3 a(endA[0], endA[1], endA[2]);
                        const Vec3 b(endB[0], endB[1], endB[2]);

                        float te;
                        if(RayIntersectsSegmentCapsule(localO, localD, a, b, castRadius, te) && te < bestT)
                        {
                            bestT = te;
                            miss  = false;
                            // normal: from the nearest point on the edge out to the hit
                            const Vec3 hitPoint = localO + localD * te;
                            const Vec3 ab       = b - a;
                            const float along   = Maths::Clamp(Maths::Dot(hitPoint - a, ab) / Maths::Max(Maths::Dot(ab, ab), 1e-8f), 0.0f, 1.0f);
                            localNormal         = (hitPoint - (a + ab * along)).Normalised();
                        }
                    }
                    tNear = bestT;
                }
            }

            // tNear < 0: the ray starts inside the box, which does not count as a hit
            if(!miss && tNear >= 0.0f && tNear < closestDist && tNear <= query.MaxDistance)
            {

                closestDist     = tNear;
                result.Body     = &body;
                result.Shape    = shape.get();
                result.Distance = tNear;
                result.Point    = query.Origin + query.Direction * tNear;
                result.Normal   = (body.GetOrientation() * localNormal).Normalised();
            }
            break;
        }
        case CollisionShapeType::CollisionCapsule:
        {
            auto* capsule = static_cast<CapsuleCollisionShape*>(shape.get());
            float capRadius = capsule->GetRadius() + castRadius;
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
        case CollisionShapeType::CollisionTerrain:
        {
            auto* terrain = static_cast<TerrainCollisionShape*>(shape.get());
            if(!terrain->HasHeightData())
                break;

            const Vec3 bodyPos = body.GetPosition();
            const float tStart = Maths::Max(tMin, 0.0f);
            const float tEnd   = Maths::Min(tMax, query.MaxDistance);
            if(tEnd <= tStart)
                break;

            const float cell    = terrain->GetScaleXZ();
            const float stepLen = Maths::Max(cell * 0.5f, 0.01f);
            const float length  = tEnd - tStart;
            int steps           = (int)(length / stepLen);
            if(steps < 2) steps = 2;
            if(steps > 4096) steps = 4096;

            const float dt = length / (float)steps;

            Vec3 startPt  = query.Origin + query.Direction * tStart;
            Vec3 localPt  = startPt - bodyPos;
            float prevDelta = localPt.y - castRadius - terrain->GetHeightAt(localPt.x, localPt.z);
            bool hit = false;
            float hitT = 0.0f;

            for(int s = 1; s <= steps; ++s)
            {
                float t = tStart + dt * (float)s;
                Vec3 wp = query.Origin + query.Direction * t;
                Vec3 lp = wp - bodyPos;
                float delta = lp.y - castRadius - terrain->GetHeightAt(lp.x, lp.z);

                if(prevDelta >= 0.0f && delta <= 0.0f)
                {
                    // Linear interpolate between sample points for sub-cell precision.
                    float denom = (prevDelta - delta);
                    float frac  = denom > Maths::M_EPSILON ? prevDelta / denom : 0.0f;
                    hitT = t - dt + frac * dt;
                    hit  = true;
                    break;
                }
                prevDelta = delta;
            }

            if(hit && hitT < closestDist && hitT <= query.MaxDistance)
            {
                closestDist     = hitT;
                result.Body     = &body;
                result.Shape    = shape.get();
                result.Distance = hitT;
                result.Point    = query.Origin + query.Direction * hitT;
                Vec3 lp         = result.Point - bodyPos;
                result.Normal   = terrain->GetNormalAt(lp.x, lp.z);
            }
            break;
        }
        case CollisionShapeType::CollisionMesh:
        {
            // Nearest triangle through the mesh's BVH, either side. A sphere cast moves each
            // triangle's plane a radius toward the ray: exact on faces, generous along edges.
            auto* mesh         = static_cast<MeshCollisionShape*>(shape.get());
            const Mat4 world   = body.GetWorldSpaceTransform() * shape->GetLocalTransform();
            const Mat4 toLocal = Mat4::Inverse(world);
            const Vec3 origin  = Vec3(toLocal * Vec4(query.Origin, 1.0f));
            const Vec3 dir     = Vec3(toLocal * Vec4(query.Direction, 0.0f));

            float tHit;
            Vec3 normal;
            if(mesh->RayCast(origin, dir, Maths::Min(closestDist, query.MaxDistance), castRadius, tHit, normal))
            {
                closestDist     = tHit;
                result.Body     = &body;
                result.Shape    = shape.get();
                result.Distance = tHit;
                result.Point    = query.Origin + query.Direction * tHit;
                result.Normal   = Vec3(world * Vec4(normal, 0.0f));
            }
            break;
        }
        default:
        {
            // Convex shapes (pyramid, hull): clip the ray against every face plane. It is
            // inside the shape between the last plane it enters and the first it leaves.
            Hull* hull = shape->GetHull();
            if(hull && hull->GetNumFaces() >= 4 && hull->GetNumVertices() >= 4)
            {
                const Mat4 world = body.GetWorldSpaceTransform() * shape->GetLocalTransform();
                Vec3 centre(0.0f);
                for(size_t v = 0; v < hull->GetNumVertices(); v++)
                    centre += Vec3(world * Vec4(hull->GetVertex((int)v).pos, 1.0f));
                centre = centre / (float)hull->GetNumVertices();

                float tEnter = -FLT_MAX, tExit = FLT_MAX;
                Vec3 enterNormal(0.0f, 1.0f, 0.0f);
                bool miss = false;
                for(size_t f = 0; f < hull->GetNumFaces() && !miss; f++)
                {
                    const HullFace& face = hull->GetFace((int)f);
                    if(face.vert_ids.Size() < 3)
                        continue;
                    const Vec3 a = Vec3(world * Vec4(hull->GetVertex(face.vert_ids[0]).pos, 1.0f));
                    const Vec3 b = Vec3(world * Vec4(hull->GetVertex(face.vert_ids[1]).pos, 1.0f));
                    const Vec3 c = Vec3(world * Vec4(hull->GetVertex(face.vert_ids[2]).pos, 1.0f));
                    Vec3 normal  = Maths::Cross(b - a, c - a).Normalised();
                    if(Maths::Dot(normal, a - centre) < 0.0f) // make it point out of the shape
                        normal = normal * -1.0f;

                    // a sphere cast pushes each face out by the radius (edges come out square)
                    const float height = Maths::Dot(normal, query.Origin - a) - castRadius;
                    const float denom  = Maths::Dot(normal, query.Direction);
                    if(Maths::Abs(denom) < 1e-8f)
                    {
                        miss = height > 0.0f; // parallel to this face and outside it
                        continue;
                    }
                    const float tPlane = -height / denom;
                    if(denom < 0.0f)
                    {
                        if(tPlane > tEnter)
                        {
                            tEnter      = tPlane;
                            enterNormal = normal;
                        }
                    }
                    else
                        tExit = Maths::Min(tExit, tPlane);
                    miss = tEnter > tExit;
                }

                if(!miss && tEnter >= 0.0f && tEnter < closestDist && tEnter <= query.MaxDistance)
                {
                    closestDist     = tEnter;
                    result.Body     = &body;
                    result.Shape    = shape.get();
                    result.Distance = tEnter;
                    result.Point    = query.Origin + query.Direction * tEnter;
                    result.Normal   = enterNormal;
                }
                break;
            }

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

        // Distance is how far the sphere's centre travelled; Point is where its surface touched.
        if(castRadius > 0.0f && result.Hit())
            result.Point = result.Point - result.Normal * castRadius;

        return result.Hit();
    }

    static Vec3 InverseDirection(const Vec3& direction)
    {
        return Vec3(
            direction.x != 0.0f ? 1.0f / direction.x : 1e30f,
            direction.y != 0.0f ? 1.0f / direction.y : 1e30f,
            direction.z != 0.0f ? 1.0f / direction.z : 1e30f);
    }

    RaycastHit LumosPhysicsEngine::Raycast(const RaycastQuery& query) const
    {
        LUMOS_PROFILE_FUNCTION();
        RaycastHit result;
        float closestDist = query.MaxDistance;
        const Vec3 invDir = InverseDirection(query.Direction);

        for(u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RaycastHit hit;
            if(RaycastBody(m_RigidBodies[i], query, invDir, closestDist, hit))
            {
                result      = hit;
                closestDist = hit.Distance;
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
        results.Clear();
        const Vec3 invDir = InverseDirection(query.Direction);

        for(u32 i = 0; i < m_RigidBodyCount && results.Size() < maxResults; i++)
        {
            RaycastHit hit;
            if(RaycastBody(m_RigidBodies[i], query, invDir, query.MaxDistance, hit))
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
