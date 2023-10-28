#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
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

#include "Maths/Transform.h"
#include "ImGui/ImGuiUtilities.h"

#include <imgui/imgui.h>

namespace Lumos
{

    float LumosPhysicsEngine::s_UpdateTimestep = 1.0f / 60.0f;

    LumosPhysicsEngine::LumosPhysicsEngine()
        : m_IsPaused(true)
        , m_UpdateAccum(0.0f)
        , m_Gravity(glm::vec3(0.0f, -9.81f, 0.0f))
        , m_DampingFactor(0.9995f)
        , m_BroadphaseDetection(nullptr)
        , m_IntegrationType(IntegrationType::RUNGE_KUTTA_4)
        , m_RootBody(nullptr)
    {
        m_DebugName = "Lumos3DPhysicsEngine";
        m_BroadphaseCollisionPairs.reserve(1000);

        m_Allocator = new PoolAllocator<RigidBody3D>();
        m_Arena     = ArenaAlloc(Megabytes(1));
    }

    void LumosPhysicsEngine::SetDefaults()
    {
        m_IsPaused        = true;
        s_UpdateTimestep  = 1.0f / 60.f;
        m_UpdateAccum     = 0.0f;
        m_Gravity         = glm::vec3(0.0f, -9.81f, 0.0f);
        m_DampingFactor   = 0.9995f;
        m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
    }

    LumosPhysicsEngine::~LumosPhysicsEngine()
    {
        CollisionDetection::Release();
    }

    void LumosPhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_IsPaused)
        {
            auto& registry    = scene->GetRegistry();
            m_ConstraintCount = 0;
            ArenaClear(m_Arena);

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Spring Constraints");
                auto viewSpring = registry.view<SpringConstraintComponent>();
                auto viewAxis   = registry.view<AxisConstraintComponent>();
                auto viewDis    = registry.view<DistanceConstraintComponent>();
                auto viewWeld   = registry.view<WeldConstraintComponent>();

                m_ConstraintCount = (uint32_t)viewSpring.size() + (uint32_t)viewAxis.size() + (uint32_t)viewDis.size() + (uint32_t)viewWeld.size();
                m_Constraints     = PushArrayNoZero(m_Arena, Constraint*, m_ConstraintCount);

                uint32_t constraintIndex = 0;
                for(auto entity : viewSpring)
                {
                    m_Constraints[constraintIndex++] = viewSpring.get<SpringConstraintComponent>(entity).GetConstraint();
                }

                for(auto entity : viewAxis)
                {
                    auto constraint = viewAxis.get<AxisConstraintComponent>(entity);

                    if(constraint.GetEntityID() != Entity(entity, Application::Get().GetCurrentScene()).GetID())
                        constraint.SetEntity(Entity(entity, Application::Get().GetCurrentScene()).GetID());
                    if(constraint.GetConstraint())
                        m_Constraints[constraintIndex++] = constraint.GetConstraint().get();
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
                for(uint32_t i = 0; (m_UpdateAccum >= s_UpdateTimestep) && i < m_MaxUpdatesPerFrame; ++i)
                {
                    m_UpdateAccum -= s_UpdateTimestep;
                    UpdatePhysics();
                }

                if(m_UpdateAccum >= s_UpdateTimestep)
                {
                    LUMOS_LOG_WARN("Physics too slow to run in real time!");
                    // Drop Time in the hope that it can continue to run in real-time
                    m_UpdateAccum = 0.0f;
                }
            }

            m_ConstraintCount = 0;
        }
    }

    void LumosPhysicsEngine::UpdatePhysics()
    {
        m_ManifoldCount = 0;
        m_Manifolds     = PushArrayNoZero(m_Arena, Manifold, 100);

        // Check for collisions
        BroadPhaseCollisions();
        NarrowPhaseCollisions();

        // Solve collision constraints
        SolveConstraints();

        // Update movement
        UpdateRigidBodys();

        RigidBody3D* current = m_RootBody;
        while(current)
        {
            current->RestTest();
            current = current->m_Next;
        }
    }

    void LumosPhysicsEngine::UpdateRigidBodys()
    {
        LUMOS_PROFILE_SCOPE("Update Rigid Body");

        m_Stats.StaticCount    = 0;
        m_Stats.RestCount      = 0;
        m_Stats.RigidBodyCount = 0;

        RigidBody3D* current = m_RootBody;
        while(current)
        {
            if(current->m_AtRest)
                m_Stats.RestCount++;
            if(current->m_Static)
                m_Stats.StaticCount++;

            m_Stats.RigidBodyCount++;

            UpdateRigidBody(current);
            current = current->m_Next;
        }
    }

    RigidBody3D* LumosPhysicsEngine::CreateBody(const RigidBody3DProperties& properties)
    {
        void* mem = m_Allocator->Allocate();

        RigidBody3D* body = new(mem) RigidBody3D();
        // Add to world doubly linked list.
        body->m_Prev = nullptr;
        body->m_Next = m_RootBody;
        if(m_RootBody)
        {
            m_RootBody->m_Prev = body;
        }
        m_RootBody = body;

        return body;
    }

    void LumosPhysicsEngine::DestroyBody(RigidBody3D* body)
    {
        // Remove world body list.
        if(body->m_Prev)
        {
            body->m_Prev->m_Next = body->m_Next;
        }

        if(body->m_Next)
        {
            body->m_Next->m_Prev = body->m_Prev;
        }

        if(body == m_RootBody)
        {
            m_RootBody = body->m_Next;
        }

        body->~RigidBody3D();
        m_Allocator->Deallocate(body);
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
                trans.SetLocalPosition(phys.GetRigidBody()->GetPosition());
                trans.SetLocalOrientation(phys.GetRigidBody()->GetOrientation());
            }
        };
    }

    glm::quat QuatMulVec3(const glm::quat& quat, const glm::vec3& b)
    {
        glm::quat ans;

        ans.w = -(quat.x * b.x) - (quat.y * b.y) - (quat.z * b.z);

        ans.x = (quat.w * b.x) + (b.y * quat.z) - (b.z * quat.y);
        ans.y = (quat.w * b.y) + (b.z * quat.x) - (b.x * quat.z);
        ans.z = (quat.w * b.z) + (b.x * quat.y) - (b.y * quat.x);

        return ans;
    }

    void LumosPhysicsEngine::UpdateRigidBody(RigidBody3D* obj) const
    {
        LUMOS_PROFILE_FUNCTION_LOW();

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
                obj->m_Orientation += obj->m_Orientation * glm::quat(obj->m_AngularVelocity * s_UpdateTimestep);
                // obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation = glm::normalize(obj->m_Orientation);

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
                obj->m_Orientation = glm::normalize(obj->m_Orientation);

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
                obj->m_Orientation = glm::normalize(obj->m_Orientation);

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
                obj->m_Orientation = glm::normalize(obj->m_Orientation);

                break;
            }
            }

            // Mark cached world transform and AABB as invalid
            obj->m_wsTransformInvalidated = true;
            obj->m_wsAabbInvalidated      = true;
        }

        s_UpdateTimestep *= m_PositionIterations;
    }

    glm::quat AngularVelcityToQuaternion(const glm::vec3& angularVelocity)
    {
        glm::quat q;
        q.x = 0.5f * angularVelocity.x;
        q.y = 0.5f * angularVelocity.y;
        q.z = 0.5f * angularVelocity.z;
        q.w = 0.5f * glm::length(angularVelocity);
        return q;
    }

    void LumosPhysicsEngine::BroadPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        m_BroadphaseCollisionPairs.clear();
        if(m_BroadphaseDetection)
            m_BroadphaseDetection->FindPotentialCollisionPairs(m_RootBody, m_BroadphaseCollisionPairs);

#ifdef CHECK_COLLISION_PAIR_DUPLICATES

        uint32_t duplicatePairs = 0;
        for(size_t i = 0; i < m_BroadphaseCollisionPairs.size(); ++i)
        {
            auto& pair = m_BroadphaseCollisionPairs[i];
            for(size_t j = i + 1; j < m_BroadphaseCollisionPairs.size(); ++j)
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

        LUMOS_LOG_INFO(duplicatePairs);
#endif
    }

    void LumosPhysicsEngine::NarrowPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_BroadphaseCollisionPairs.empty())
            return;

        m_Stats.NarrowPhaseCount = (uint32_t)m_BroadphaseCollisionPairs.size();
        m_Stats.CollisionCount   = 0;

        for(auto& cp : m_BroadphaseCollisionPairs)
        {
            auto shapeA = cp.pObjectA->GetCollisionShape();
            auto shapeB = cp.pObjectB->GetCollisionShape();

            if(shapeA && shapeB)
            {
                CollisionData colData;

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
                        manifold.Initiate(cp.pObjectA, cp.pObjectB);

                        // Construct contact points that form the perimeter of the collision manifold
                        if(CollisionDetection::Get().BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, &manifold))
                        {
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
            m_BroadphaseDetection = Lumos::CreateSharedPtr<OctreeBroadphase>(5, 5);
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

        // Draw all constraints
        if(m_DebugDrawFlags & PhysicsDebugFlags::CONSTRAINT)
        {
            for(uint32_t index = 0; index < m_ConstraintCount; index++)
                m_Constraints[index]->DebugDraw();
        }

        if(!m_IsPaused && m_BroadphaseDetection && (m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE))
            m_BroadphaseDetection->DebugDraw();

        RigidBody3D* current = m_RootBody;
        while(current)
        {
            current->DebugDraw(m_DebugDrawFlags);
            if(current->GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                current->GetCollisionShape()->DebugDraw(current);
            current = current->m_Next;
        }
    }
}
