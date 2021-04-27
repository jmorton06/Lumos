#include "Precompiled.h"
#include "LumosPhysicsEngine.h"
#include "CollisionDetection.h"
#include "RigidBody3D.h"
#include "Core/OS/Window.h"

#include "Integration.h"
#include "Constraint.h"
#include "Utilities/TimeStep.h"
#include "Core/JobSystem.h"

#include "Core/Application.h"
#include "Scene/Component/Physics3DComponent.h"

#include "Maths/Transform.h"

#include <imgui/imgui.h>

#define THREAD_RIGID_BODY_UPDATE
#define THREAD_APPLY_IMPULSES
#define THREAD_NARROWPHASE

namespace Lumos
{

    float LumosPhysicsEngine::s_UpdateTimestep = 1.0f / 60.0f;

    LumosPhysicsEngine::LumosPhysicsEngine()
        : m_IsPaused(true)
        , m_UpdateAccum(0.0f)
        , m_Gravity(Maths::Vector3(0.0f, -9.81f, 0.0f))
        , m_DampingFactor(0.999f)
        , m_BroadphaseDetection(nullptr)
        , m_IntegrationType(IntegrationType::RUNGE_KUTTA_4)
    {
        m_DebugName = "Lumos3DPhysicsEngine";
        m_RigidBodys.reserve(100);
        m_BroadphaseCollisionPairs.reserve(1000);
        m_Manifolds.reserve(100);
    }

    void LumosPhysicsEngine::SetDefaults()
    {
        m_IsPaused = true;
        s_UpdateTimestep = 1.0f / 60.f;
        m_UpdateAccum = 0.0f;
        m_Gravity = Maths::Vector3(0.0f, -9.81f, 0.0f);
        m_DampingFactor = 0.999f;
        m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
    }

    LumosPhysicsEngine::~LumosPhysicsEngine()
    {
        m_RigidBodys.clear();
        m_Constraints.clear();
        m_Manifolds.clear();

        CollisionDetection::Release();
    }

    void LumosPhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_RigidBodys.clear();

        if(!m_IsPaused)
        {
            auto& registry = scene->GetRegistry();
            auto group = registry.group<Physics3DComponent>(entt::get<Maths::Transform>);

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Rigid Bodies");
                for(auto entity : group)
                {
                    const auto& phys = group.get<Physics3DComponent>(entity);
                    auto& physicsObj = phys.GetRigidBody();
                    m_RigidBodys.push_back(physicsObj.get());
                };
            }

            if(m_RigidBodys.empty())
            {
                return;
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Spring Constraints");
                m_Constraints.clear();

                auto viewSpring = registry.view<SpringConstraintComponent>();

                for(auto entity : viewSpring)
                {
                    const auto& constraint = viewSpring.get<SpringConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Axis Constraints");

                auto viewAxis = registry.view<AxisConstraintComponent>();

                for(auto entity : viewAxis)
                {
                    const auto& constraint = viewAxis.get<AxisConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Distance Constraints");
                auto viewDis = registry.view<DistanceConstraintComponent>();

                for(auto entity : viewDis)
                {
                    const auto& constraint = viewDis.get<DistanceConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::Get Weld Constraints");
                auto viewWeld = registry.view<WeldConstraintComponent>();

                for(auto entity : viewWeld)
                {
                    const auto& constraint = viewWeld.get<WeldConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::UpdatePhysics");
                if(m_MultipleUpdates)
                {
                    const int max_updates_per_frame = 5;

                    m_UpdateAccum += timeStep.GetSeconds();
                    for(int i = 0; (m_UpdateAccum >= s_UpdateTimestep) && i < max_updates_per_frame; ++i)
                    {
                        m_UpdateAccum -= s_UpdateTimestep;
                        UpdatePhysics();
                    }

                    if(m_UpdateAccum >= s_UpdateTimestep)
                    {
                        LUMOS_LOG_WARN("Physics too slow to run in real time!");
                        //Drop Time in the hope that it can continue to run in real-time
                        m_UpdateAccum = 0.0f;
                    }
                }
                else
                {
                    s_UpdateTimestep = timeStep.GetSeconds();
                    UpdatePhysics();
                }
            }

            {
                LUMOS_PROFILE_SCOPE("Physics::Set Transforms");

                for(auto entity : group)
                {
                    const auto& [phys, trans] = group.get<Physics3DComponent, Maths::Transform>(entity);

                    trans.SetLocalPosition(phys.GetRigidBody()->GetPosition());
                    trans.SetLocalOrientation(phys.GetRigidBody()->GetOrientation());
                };
            }
            m_Constraints.clear();
        }
    }

    void LumosPhysicsEngine::UpdatePhysics()
    {
        m_Manifolds.clear();

        //Check for collisions
        BroadPhaseCollisions();
        NarrowPhaseCollisions();

        //Solve collision constraints
        SolveConstraints();

        //Update movement
        UpdateRigidBodys();
    }

    void LumosPhysicsEngine::UpdateRigidBodys()
    {
#ifdef THREAD_RIGID_BODY_UPDATE
        LUMOS_PROFILE_SCOPE("Thread Update Rigid Body");
        System::JobSystem::Context jobSystemContext;
        System::JobSystem::Dispatch(jobSystemContext, static_cast<uint32_t>(m_RigidBodys.size()), 128, [&](JobDispatchArgs args)
            { UpdateRigidBody(m_RigidBodys[args.jobIndex]); });

        System::JobSystem::Wait(jobSystemContext);
#else
        LUMOS_PROFILE_SCOPE("Update Rigid Body");

        for(int i = 0; i < m_RigidBodys.size(); i++)
            UpdateRigidBody(m_RigidBodys[i]);
#endif
    }

    void LumosPhysicsEngine::UpdateRigidBody(RigidBody3D* obj) const
    {
        LUMOS_PROFILE_FUNCTION();
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
                obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation.Normalise();

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

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
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

                // Update orientation
                obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_2:
            {
                // RK2 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK2(state, 0.0f, s_UpdateTimestep);

                obj->m_Position = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

                // Update orientation
                obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation.Normalise();

                break;
            }

            case IntegrationType::RUNGE_KUTTA_4:
            {
                // RK4 integration for linear motion
                Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK4(state, 0.0f, s_UpdateTimestep);
                obj->m_Position = state.position;
                obj->m_LinearVelocity = state.velocity;

                // Linear velocity damping
                obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                // Update angular velocity
                obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                // Angular velocity damping
                obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

                // Update orientation
                obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                obj->m_Orientation.Normalise();

                break;
            }
            }

            // Mark cached world transform and AABB as invalid
            obj->m_wsTransformInvalidated = true;
            obj->m_wsAabbInvalidated = true;

            obj->RestTest();
        }
    }

    void LumosPhysicsEngine::BroadPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        m_BroadphaseCollisionPairs.clear();
        if(m_BroadphaseDetection)
            m_BroadphaseDetection->FindPotentialCollisionPairs(m_RigidBodys.data(), (uint32_t)m_RigidBodys.size(), m_BroadphaseCollisionPairs);
    }

    void LumosPhysicsEngine::NarrowPhaseCollisions()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_BroadphaseCollisionPairs.empty())
            return;
#ifdef THREAD_NARROWPHASE
        System::JobSystem::Context jobSystemContext;
        System::JobSystem::Dispatch(jobSystemContext, static_cast<uint32_t>(m_BroadphaseCollisionPairs.size()), 128, [&](JobDispatchArgs args)
#else
        for(auto& cp : m_BroadphaseCollisionPairs)
#endif
            {
#ifdef THREAD_NARROWPHASE
                CollisionPair& cp = m_BroadphaseCollisionPairs[args.jobIndex];
#endif
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
                            Manifold& manifold = m_Manifolds.emplace_back();
                            manifold.Initiate(cp.pObjectA, cp.pObjectB);

                            // Construct contact points that form the perimeter of the collision manifold
                            if(CollisionDetection::Get().BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, &manifold))
                            {
                                // Fire callback
                                cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, &manifold);
                                cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, &manifold);
                            }
                            else
                            {
                                m_Manifolds.pop_back();
                            }

                            m_ManifoldLock.unlock();
                        }
                    }
                }
            }
#ifdef THREAD_NARROWPHASE
        );

        System::JobSystem::Wait(jobSystemContext);
#endif
    }

    void LumosPhysicsEngine::SolveConstraints()
    {
        LUMOS_PROFILE_FUNCTION();

        {
            LUMOS_PROFILE_SCOPE("Solve Manifolds");

            for(Manifold& m : m_Manifolds)
                m.PreSolverStep(s_UpdateTimestep);
        }
        {
            LUMOS_PROFILE_SCOPE("Solve Constraints");

            for(Constraint* c : m_Constraints)
                c->PreSolverStep(s_UpdateTimestep);
        }

        {
            LUMOS_PROFILE_SCOPE("Apply Impulses");
#ifdef THREAD_APPLY_IMPULSES
            System::JobSystem::Context jobSystemContext;
            System::JobSystem::Dispatch(jobSystemContext, static_cast<uint32_t>(SOLVER_ITERATIONS), 128, [&](JobDispatchArgs args)
#else
            for(int i = 0; i < SOLVER_ITERATIONS; i++)
#endif
                {
                    for(
                        Manifold& m : m_Manifolds)
                    {
                        m.ApplyImpulse();
                    }

                    for(Constraint* c : m_Constraints)
                    {
                        c->ApplyImpulse();
                    }
                }
#ifdef THREAD_APPLY_IMPULSES
            );
            System::JobSystem::Wait(jobSystemContext);
#endif
        }
    }

    void LumosPhysicsEngine::ClearConstraints()
    {
        m_Constraints.clear();
    }

    std::string IntegrationTypeToString(IntegrationType type)
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

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Rigid Bodys");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", GetNumberRigidBodys());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Constraints");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", static_cast<int>(m_Constraints.size()));
        ImGui::PopItemWidth();
        ImGui::NextColumn();

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
        LUMOS_PROFILE_FUNCTION();
        if(m_DebugDrawFlags & PhysicsDebugFlags::MANIFOLD)
        {
            for(Manifold& m : m_Manifolds)
                m.DebugDraw();
        }

        // Draw all constraints
        if(m_DebugDrawFlags & PhysicsDebugFlags::CONSTRAINT)
        {
            for(Constraint* c : m_Constraints)
                c->DebugDraw();
        }

        if(m_BroadphaseDetection && (m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE))
            m_BroadphaseDetection->DebugDraw();

        auto scene = Application::Get().GetCurrentScene();
        auto& registry = scene->GetRegistry();

        auto group = registry.group<Physics3DComponent>(entt::get<Maths::Transform>);

        if(group.empty())
            return;

        for(auto entity : group)
        {
            const auto& phys = group.get<Physics3DComponent>(entity);

            auto& physicsObj = phys.GetRigidBody();

            if(physicsObj)
            {
                physicsObj->DebugDraw(m_DebugDrawFlags);
                if(physicsObj->GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                    physicsObj->GetCollisionShape()->DebugDraw(physicsObj.get());
            }
        }
    }
}
