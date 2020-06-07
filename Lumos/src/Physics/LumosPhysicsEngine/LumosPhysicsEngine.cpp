#include "lmpch.h"
#include "LumosPhysicsEngine.h"
#include "CollisionDetection.h"
#include "PhysicsObject3D.h"
#include "Core/OS/Window.h"

#include "Integration.h"
#include "Constraint.h"
#include "Utilities/TimeStep.h"
#include "Core/JobSystem.h"
#include "Core/Profiler.h"

#include "ECS/Component/Physics3DComponent.h"
#include "Maths/Transform.h"

#include <imgui/imgui.h>

namespace Lumos
{

    float LumosPhysicsEngine::s_UpdateTimestep = 1.0f/60.0f;
    
	LumosPhysicsEngine::LumosPhysicsEngine()
		: m_IsPaused(true)
		, m_UpdateAccum(0.0f)
		, m_Gravity(Maths::Vector3(0.0f, -9.81f, 0.0f))
		, m_DampingFactor(0.999f)
		, m_BroadphaseDetection(nullptr)
		, m_IntegrationType(IntegrationType::RUNGE_KUTTA_4)
	{
        m_DebugName = "Lumos3DPhysicsEngine";
		m_PhysicsObjects.reserve(100);
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
        m_PhysicsObjects.clear();
        
        for (Constraint* c : m_Constraints)
            delete c;
        m_Constraints.clear();
        
        for (Manifold* m : m_Manifolds)
            delete m;
        m_Manifolds.clear();
        
		CollisionDetection::Release();
	}

	void LumosPhysicsEngine::OnUpdate(const TimeStep& timeStep, Scene* scene)
	{
        LUMOS_PROFILE_BLOCK("LumosPhysicsEngine::OnUpdate");
        m_PhysicsObjects.clear();

		if (!m_IsPaused)
		{
            auto& registry = scene->GetRegistry();
            
            auto group = registry.group<Physics3DComponent>(entt::get<Maths::Transform>);

            if (group.empty())
                return;
            
            for(auto entity : group)
            {
                const auto &phys = group.get<Physics3DComponent>(entity);

                auto& physicsObj = phys.GetPhysicsObject();
                               
                if(physicsObj)
                    m_PhysicsObjects.emplace_back(physicsObj);
            };
            
			if (m_MultipleUpdates)
			{
				const int max_updates_per_frame = 5;

				m_UpdateAccum += timeStep.GetMillis();
				for (int i = 0; (m_UpdateAccum >= s_UpdateTimestep) && i < max_updates_per_frame; ++i)
				{
					m_UpdateAccum -= s_UpdateTimestep;
					UpdatePhysics(scene);
				}

				if (m_UpdateAccum >= s_UpdateTimestep)
				{
					Debug::Log::Error("Physics too slow to run in real time!");
					//Drop Time in the hope that it can continue to run in real-time
					m_UpdateAccum = 0.0f;
				}
			}
			else
			{
				s_UpdateTimestep = timeStep.GetMillis();
				UpdatePhysics(scene);
			}
            
            for(auto entity : group)
            {
                const auto &[phys,trans] = group.get<Physics3DComponent, Maths::Transform>(entity);

                trans.SetLocalPosition(phys.GetPhysicsObject()->GetPosition());
                trans.SetLocalOrientation(phys.GetPhysicsObject()->GetOrientation());
            };
		}
	}

	void LumosPhysicsEngine::UpdatePhysics(Scene* scene)
	{
        
		for (Manifold* m : m_Manifolds)
		{
			delete m;
		}
		m_Manifolds.clear();

		//Check for collisions
		BroadPhaseCollisions();
		NarrowPhaseCollisions();
		
		//Solve collision constraints
		SolveConstraints();
		
		//Update movement
		UpdatePhysicsObjects();
	}

	void LumosPhysicsEngine::UpdatePhysicsObjects()
	{
        System::JobSystem::Dispatch(static_cast<u32>(m_PhysicsObjects.size()), 4, [&](JobDispatchArgs args)
        {
            UpdatePhysicsObject(m_PhysicsObjects[args.jobIndex]);
        });
        
        System::JobSystem::Wait();
	}

	void LumosPhysicsEngine::UpdatePhysicsObject(const Ref<PhysicsObject3D>& obj) const
	{
		if (!obj->GetIsStatic() && obj->IsAwake())
		{
			const float damping = m_DampingFactor;

			// Apply gravity
			if (obj->m_InvMass > 0.0f)
				obj->m_LinearVelocity += m_Gravity * s_UpdateTimestep;

			switch (m_IntegrationType)
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
				obj->m_Orientation.Normalize();

				// Update angular velocity
				obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

				// Angular velocity damping
				obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

				break;
			}

			default:
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
				obj->m_Orientation.Normalize();

				break;
			}

			case IntegrationType::RUNGE_KUTTA_2:
			{
				// RK2 integration for linear motion
				Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
                Integration::RK2(state,0.0f, s_UpdateTimestep);

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
				obj->m_Orientation.Normalize();

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
				obj->m_Orientation.Normalize();

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
		m_BroadphaseCollisionPairs.clear();
		if (m_BroadphaseDetection)
			m_BroadphaseDetection->FindPotentialCollisionPairs(m_PhysicsObjects, m_BroadphaseCollisionPairs);
	}

	void LumosPhysicsEngine::NarrowPhaseCollisions()
	{
		if (!m_BroadphaseCollisionPairs.empty())
		{
			CollisionData colData;
			CollisionDetection colDetect;

			//System::JobSystem::Dispatch(static_cast<u32>(m_BroadphaseCollisionPairs.size()), 4, [&](JobDispatchArgs args)
            for(auto& cp : m_BroadphaseCollisionPairs)
			{
				//CollisionPair &cp = m_BroadphaseCollisionPairs[args.jobIndex];
				auto shapeA = cp.pObjectA->GetCollisionShape();
				auto shapeB = cp.pObjectB->GetCollisionShape();

				if (shapeA && shapeB)
				{
					// Detects if the objects are colliding - Seperating Axis Theorem
					if (CollisionDetection::Instance()->CheckCollision(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), &colData))
					{
						// Check to see if any of the objects have collision callbacks that dont
						// want the objects to physically collide
						const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
						const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

						if (okA && okB)
						{
							// Build full collision manifold that will also handle the collision
							// response between the two objects in the solver stage
							Manifold* manifold = lmnew Manifold();
							manifold->Initiate(cp.pObjectA, cp.pObjectB);

							// Construct contact points that form the perimeter of the collision manifold
							if (CollisionDetection::Instance()->BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), colData, manifold))
							{
								// Fire callback
								cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, manifold);
								cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, manifold);

								// Add to list of manifolds that need solving
								m_Manifolds.push_back(manifold);
							}
							else
							{
								delete manifold;
							}
						}
					}
				}
			}//);

			//System::JobSystem::Wait();
		}
	}

	void LumosPhysicsEngine::SolveConstraints()
	{
		for (Manifold* m : m_Manifolds) m->PreSolverStep(s_UpdateTimestep);
		for (Constraint* c : m_Constraints)	c->PreSolverStep(s_UpdateTimestep);

		for (size_t i = 0; i < SOLVER_ITERATIONS; ++i)
		{
			for (Manifold* m : m_Manifolds)
			{
				m->ApplyImpulse();
			}

			for (Constraint* c : m_Constraints)
			{
				c->ApplyImpulse();
			}
		}
	}

    void LumosPhysicsEngine::ClearConstraints()
    {
        for (Constraint* c : m_Constraints)
            delete c;
        m_Constraints.clear();
    }

	String IntegrationTypeToString(IntegrationType type)
	{
		switch (type)
		{
		case  IntegrationType::EXPLICIT_EULER : return "EXPLICIT EULER";
		case  IntegrationType::SEMI_IMPLICIT_EULER : return "SEMI IMPLICIT EULER";
		case  IntegrationType::RUNGE_KUTTA_2 : return "RUNGE KUTTA 2";
		case  IntegrationType::RUNGE_KUTTA_4 : return "RUNGE KUTTA 4";
		default : return "";
		}
	}

	void LumosPhysicsEngine::OnImGui()
	{
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
		ImGui::TextUnformatted("Number Of Physics Objects");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", GetNumberPhysicsObjects());
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
		if (ImGui::BeginMenu(IntegrationTypeToString(m_IntegrationType).c_str()))
		{
			if (ImGui::MenuItem("EXPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 0, true)) { m_IntegrationType = IntegrationType::EXPLICIT_EULER; }
			if (ImGui::MenuItem("SEMI IMPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 1, true)) { m_IntegrationType = IntegrationType::SEMI_IMPLICIT_EULER; }
			if (ImGui::MenuItem("RUNGE KUTTA 2", "", static_cast<int>(m_IntegrationType) == 2, true)) { m_IntegrationType = IntegrationType::RUNGE_KUTTA_2; }
			if (ImGui::MenuItem("RUNGE KUTTA 4", "", static_cast<int>(m_IntegrationType) == 3, true)) { m_IntegrationType = IntegrationType::RUNGE_KUTTA_4; }
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
        if (m_DebugDrawFlags & PhysicsDebugFlags::MANIFOLD)
        {
            for (Manifold *m : m_Manifolds)
                m->DebugDraw();
        }

        // Draw all constraints
        if (m_DebugDrawFlags & PhysicsDebugFlags::CONSTRAINT)
        {
            for (Constraint *c : m_Constraints)
                c->DebugDraw();
        }

        if (m_BroadphaseDetection && (m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE))
            m_BroadphaseDetection->DebugDraw();
    
        for(auto obj : m_PhysicsObjects)
        {
            obj->DebugDraw(m_DebugDrawFlags);
        
            if (obj->GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                obj->GetCollisionShape()->DebugDraw(obj.get());
        }
    }
}
