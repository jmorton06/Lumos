#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "RigidBody3D.h"
#include "Narrowphase/CollisionDetection.h"
#include "Broadphase/BruteForceBroadphase.h"
#include "Broadphase/OctreeBroadphase.h"
#include "RigidBody3D.h"
#include "Integration.h"
#include "Constraints/Constraint.h"
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
        m_DebugName = "Lumos3DPhysicsEngine";
        m_BroadphaseCollisionPairs.Reserve(1000);

        m_FrameArena  = ArenaAlloc(Megabytes(4));
        m_Arena       = ArenaAlloc(m_MaxRigidBodyCount * sizeof(RigidBody3D) * 2);
        m_RigidBodies = PushArray(m_Arena, RigidBody3D, m_MaxRigidBodyCount);
        m_RigidBodyFreeList = TDArray<RigidBody3D*>(m_Arena);
        m_RigidBodyFreeList.Reserve(m_MaxRigidBodyCount);
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

                m_UpdateAccum += (float)timeStep.GetSeconds();
                for(uint32_t i = 0; (m_UpdateAccum + Maths::M_EPSILON >= s_UpdateTimestep) && i < m_MaxUpdatesPerFrame; ++i)
                {
                    m_UpdateAccum -= s_UpdateTimestep;
                    UpdatePhysics();
                }

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

                // Only log if 25% behind
                if(m_AvgOverrun > s_UpdateTimestep * 0.25f)
                {
                    LWARN("Physics running behind: avg overrun = %.4f", m_AvgOverrun);
                }
            }
        }
    }

    void LumosPhysicsEngine::UpdatePhysics()
    {
        m_ManifoldCount = 0;
        m_Manifolds     = PushArrayNoZero(m_FrameArena, Manifold, 1000);

        // Check for collisions
        BroadPhaseCollisions();
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

        for (u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& current = m_RigidBodies[i];
            if (!current.m_IsValid)
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
        if (!m_RigidBodyFreeList.Empty())
        {
            RigidBody3D* body = m_RigidBodyFreeList.Back();
            *body = RigidBody3D(properties);
            body->m_IsValid = true;
            m_RigidBodyFreeList.PopBack();

            return body;
        }
        else
        {
            if (m_RigidBodyCount < m_MaxRigidBodyCount)
            {
                RigidBody3D* body = &m_RigidBodies[m_RigidBodyCount];
                *body = RigidBody3D(properties);
                body->m_IsValid = true;
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
        if (body && body->m_IsValid)
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

        s_UpdateTimestep /= m_PositionIterations;

        if(!obj->GetIsStatic() && obj->IsAwake())
        {
            const float damping = m_DampingFactor;

            // Apply gravity
            if(obj->m_InvMass > 0.0f)
                obj->m_LinearVelocity += m_Gravity * s_UpdateTimestep;

            switch(m_IntegrationType)
            {
            case IntegrationType::EXPLICIT_EULER:
            {
                // Update position
                obj->m_Position += obj->m_LinearVelocity * s_UpdateTimestep;

                // Update linear velocity (v = u + at)
                obj->m_LinearVelocity += obj->m_Force * obj->m_InvMass * s_UpdateTimestep;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update orientation
                obj->m_Orientation += obj->m_Orientation * Quat(obj->m_AngularVelocity * s_UpdateTimestep);
                // obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation.Normalise();

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                break;
            }

            case IntegrationType::SEMI_IMPLICIT_EULER:
            {
                // Update linear velocity (v = u + at)
                obj->m_LinearVelocity += obj->m_LinearVelocity * obj->m_InvMass * s_UpdateTimestep;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update position
                obj->m_Position += obj->m_LinearVelocity * s_UpdateTimestep;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep;

                // Update orientation
                obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_2:
            {
                // RK2 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK2(state, 0.0f, s_UpdateTimestep);

                obj->m_Position       = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep * 0.5f;

                // Update orientation
                obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_4:
            {
                // RK4 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK4(state, 0.0f, s_UpdateTimestep);
                obj->m_Position       = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                // Update orientation
                // Check order of quat multiplication
                auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep * 0.5f;

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

        s_UpdateTimestep *= m_PositionIterations;
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
                    // Check to see if any of the objects have collision callbacks that dont
                    // want the objects to physically collide
                    const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
                    const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

                    if(okA && okB)
                    {
                        // Build full collision manifold that will also handle the collision
                        // response between the two objects in the solver stage
                        m_ManifoldLock.lock();
                        Manifold& manifold = m_Manifolds[m_ManifoldCount++];
                        manifold.Initiate(cp.pObjectA, cp.pObjectB, m_BaumgarteScalar, m_BaumgarteSlop);

                        ASSERT(m_ManifoldCount < 1000);
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

                        m_ManifoldLock.unlock();
                    }
                }
            }
        }
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

        for (u32 i = 0; i < m_RigidBodyCount; i++)
        {
            RigidBody3D& current = m_RigidBodies[i];
            if (current.m_IsValid)
            {
                current.DebugDraw(m_DebugDrawFlags);
                if (current.GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                    current.GetCollisionShape()->DebugDraw(&current);
            }
        }
    }
}
