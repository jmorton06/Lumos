#include "LM.h"
#include "LumosPhysicsEngine.h"
#include "CollisionDetection.h"
#include "PhysicsObject3D.h"
#include "App/Window.h"

#include "Integration.h"
#include "Constraint.h"
#include "Entity/Entity.h"
#include "Utilities/TimeStep.h"
#include "System/JobSystem.h"
#include "System/Profiler.h"

#include <imgui/imgui.h>

namespace lumos
{

	LumosPhysicsEngine::LumosPhysicsEngine()
		: m_IsPaused(true)
		, m_UpdateTimestep(1.0f / 60.f)
		, m_UpdateAccum(0.0f)
		, m_Gravity(maths::Vector3(0.0f, -9.81f, 0.0f))
		, m_DampingFactor(0.999f)
		, m_BroadphaseDetection(nullptr)
		, m_IntegrationType(IntegrationType::RUNGE_KUTTA_4)
	{
        m_DebugName = "Lumos3DPhysicsEngine";
	}

	void LumosPhysicsEngine::SetDefaults()
	{
		m_IsPaused = true;
		m_UpdateTimestep = 1.0f / 60.f;
		m_UpdateAccum = 0.0f;
		m_Gravity = maths::Vector3(0.0f, -9.81f, 0.0f);
		m_DampingFactor = 0.999f;
		m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
	}

	LumosPhysicsEngine::~LumosPhysicsEngine()
	{
		RemoveAllPhysicsObjects();

		CollisionDetection::Release();

		if (m_BroadphaseDetection != nullptr)
			delete m_BroadphaseDetection;
	}

	void LumosPhysicsEngine::AddPhysicsObject(std::shared_ptr<PhysicsObject3D> obj)
	{
		m_PhysicsObjects.push_back(obj);
	}

	void LumosPhysicsEngine::RemovePhysicsObject(std::shared_ptr<PhysicsObject3D> obj)
	{
		//// Lookup the object in question
		const auto it = std::find(m_PhysicsObjects.begin(), m_PhysicsObjects.end(), obj);

		// If found, remove it from the list
		if (it != m_PhysicsObjects.end())
			m_PhysicsObjects.erase(it);
	}

	void LumosPhysicsEngine::RemoveAllPhysicsObjects()
	{
		//delete and remove all constraints/collision manifolds
		for (Constraint* c : m_Constraints)
			delete c;
		m_Constraints.clear();

		for (Manifold* m : m_Manifolds)
			delete m;
		m_Manifolds.clear();

		m_PhysicsObjects.clear();
	}

	void LumosPhysicsEngine::OnUpdate(TimeStep* timeStep)
	{
		if (!m_IsPaused)
		{
			if (m_MultipleUpdates)
			{
				const int max_updates_per_frame = 5;

				m_UpdateAccum += timeStep->GetSeconds();
				for (int i = 0; (m_UpdateAccum >= m_UpdateTimestep) && i < max_updates_per_frame; ++i)
				{
					m_UpdateAccum -= m_UpdateTimestep;
					UpdatePhysics();
				}

				if (m_UpdateAccum >= m_UpdateTimestep)
				{
					LUMOS_CORE_ERROR("Physics too slow to run in real time!");
					//Drop Time in the hope that it can continue to run in real-time
					m_UpdateAccum = 0.0f;
				}

			}
			else
			{
				m_UpdateTimestep = timeStep->GetSeconds();
				UpdatePhysics();
			}
		}
	}

	void LumosPhysicsEngine::UpdatePhysics()
	{
		for (Manifold* m : m_Manifolds)
		{
			delete m;
		}
		m_Manifolds.clear();

		//Check for collisions
		LUMOS_PROFILE(system::Profiler::OnBeginRange("BroadPhase", true, "Lumos3DPhysicsEngine"));
		BroadPhaseCollisions();
		LUMOS_PROFILE(system::Profiler::OnEndRange("BroadPhase", true, "Lumos3DPhysicsEngine"));
		
		LUMOS_PROFILE(system::Profiler::OnBeginRange("NarrowPhase", true, "Lumos3DPhysicsEngine"));
		NarrowPhaseCollisions();
		LUMOS_PROFILE(system::Profiler::OnEndRange("NarrowPhase", true, "Lumos3DPhysicsEngine"));
		
		//Solve collision constraints
		LUMOS_PROFILE(system::Profiler::OnBeginRange("SolveConstraints", true, "Lumos3DPhysicsEngine"));
		SolveConstraints();
		LUMOS_PROFILE(system::Profiler::OnEndRange("SolveConstraints", true, "Lumos3DPhysicsEngine"));
		
		//Update movement
		LUMOS_PROFILE(system::Profiler::OnBeginRange("UpdatePhysicsObjects", true, "Lumos3DPhysicsEngine"));
		UpdatePhysicsObjects();
		LUMOS_PROFILE(system::Profiler::OnEndRange("UpdatePhysicsObjects", true, "Lumos3DPhysicsEngine"));
	}

	void LumosPhysicsEngine::DebugRender(uint64 debugFlags)
	{
		// Draw all collision manifolds
		if (debugFlags & DEBUGDRAW_FLAGS_MANIFOLD)
		{
			for (Manifold *m : m_Manifolds)
				m->DebugDraw();
		}

		// Draw all constraints
		if (debugFlags & DEBUGDRAW_FLAGS_CONSTRAINT)
		{
			for (Constraint *c : m_Constraints)
				c->DebugDraw();
		}

		if (m_BroadphaseDetection && (debugFlags & DEBUGDRAW_FLAGS_BROADPHASE))
			m_BroadphaseDetection->DebugDraw();
	}

	void LumosPhysicsEngine::UpdatePhysicsObjects()
	{
        system::JobSystem::Dispatch(static_cast<uint32>(m_PhysicsObjects.size()), 16, [&](JobDispatchArgs args)
        {
            UpdatePhysicsObject(m_PhysicsObjects[args.jobIndex].get());
        });
        
        system::JobSystem::Wait();
	}

	void LumosPhysicsEngine::UpdatePhysicsObject(PhysicsObject3D* obj) const
	{
		if (!obj->GetIsStatic() && obj->IsAwake())
		{
			const float damping = m_DampingFactor;

			// Apply gravity
			if (obj->m_InvMass > 0.0f)
				obj->m_LinearVelocity += m_Gravity * m_UpdateTimestep;

			switch (m_IntegrationType)
			{
			case IntegrationType::EXPLICIT_EULER:
			{
				// Update position
				obj->m_Position += obj->m_LinearVelocity * m_UpdateTimestep;

				// Update linear velocity (v = u + at)
				obj->m_LinearVelocity += obj->m_Force * obj->m_InvMass * m_UpdateTimestep;

				// Linear velocity damping
				obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

				// Update orientation
				obj->m_Orientation = obj->m_Orientation + (obj->m_Orientation * (obj->m_AngularVelocity * m_UpdateTimestep * 0.5f));
				obj->m_Orientation.Normalise();

				// Update angular velocity
				obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * m_UpdateTimestep;

				// Angular velocity damping
				obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

				break;
			}

			default:
			case IntegrationType::SEMI_IMPLICIT_EULER:
			{
				// Update linear velocity (v = u + at)
				obj->m_LinearVelocity += obj->m_LinearVelocity * obj->m_InvMass * m_UpdateTimestep;

				// Linear velocity damping
				obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

				// Update position
				obj->m_Position += obj->m_LinearVelocity * m_UpdateTimestep;

				// Update angular velocity
				obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * m_UpdateTimestep;

				// Angular velocity damping
				obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

				// Update orientation
				obj->m_Orientation = obj->m_Orientation + (obj->m_Orientation * (obj->m_AngularVelocity * m_UpdateTimestep * 0.5f));
				obj->m_Orientation.Normalise();

				break;
			}

			case IntegrationType::RUNGE_KUTTA_2:
			{
				// RK2 integration for linear motion
				Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
				Integration::RK2(state,0.0f, m_UpdateTimestep);
				obj->m_Position = state.position;
				obj->m_LinearVelocity = state.velocity;

				// Linear velocity damping
				obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

				// Update angular velocity
				obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * m_UpdateTimestep;

				// Angular velocity damping
				obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

				// Update orientation
				obj->m_Orientation = obj->m_Orientation + (obj->m_Orientation * (obj->m_AngularVelocity * m_UpdateTimestep * 0.5f));
				obj->m_Orientation.Normalise();

				break;
			}

			case IntegrationType::RUNGE_KUTTA_4:
			{
				// RK4 integration for linear motion
				Integration::State state = { obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass };
				Integration::RK4(state, 0.0f, m_UpdateTimestep);
				obj->m_Position = state.position;
				obj->m_LinearVelocity = state.velocity;

				// Linear velocity damping
				obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

				// Update angular velocity
				obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * m_UpdateTimestep;

				// Angular velocity damping
				obj->m_AngularVelocity = obj->m_AngularVelocity * damping;

				// Update orientation
				obj->m_Orientation = obj->m_Orientation + (obj->m_Orientation * (obj->m_AngularVelocity * m_UpdateTimestep * 0.5f));
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

			for (size_t i = 0; i < m_BroadphaseCollisionPairs.size(); ++i)
			{
				CollisionPair &cp = m_BroadphaseCollisionPairs[i];
				CollisionShape* shapeA = cp.pObjectA->GetCollisionShape();
				CollisionShape* shapeB = cp.pObjectB->GetCollisionShape();

				if (!shapeA || !shapeB)
					continue;

				// Detects if the objects are colliding - Seperating Axis Theorem
				if (CollisionDetection::Instance()->CheckCollision(cp.pObjectA, cp.pObjectB, shapeA, shapeB, &colData))
				{
					// Check to see if any of the objects have collision callbacks that dont
					// want the objects to physically collide
					const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
					const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

					if (okA && okB)
					{
						// Build full collision manifold that will also handle the collision
						// response between the two objects in the solver stage
						Manifold* manifold = new Manifold();
						manifold->Initiate(cp.pObjectA, cp.pObjectB);

						// Construct contact points that form the perimeter of the collision manifold
						if (CollisionDetection::Instance()->BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA, shapeB, colData, manifold))
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
		}
	}

	void LumosPhysicsEngine::SolveConstraints()
	{
		for (Manifold* m : m_Manifolds)		m->PreSolverStep(m_UpdateTimestep);
		for (Constraint* c : m_Constraints)	c->PreSolverStep(m_UpdateTimestep);

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

	PhysicsObject3D* LumosPhysicsEngine::FindObjectByName(const String& name)
	{
		auto it = std::find_if(m_PhysicsObjects.begin(), m_PhysicsObjects.end(), [name](std::shared_ptr<PhysicsObject3D> o) 
		{
			Entity *po = o->GetAssociatedObject();
			return (po != nullptr && po->GetName() == name);
		});

		return (it == m_PhysicsObjects.end()) ? nullptr : (*it).get();
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

	void LumosPhysicsEngine::OnIMGUI()
	{
		ImGui::Text("3D Physics Engine");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::Columns(2);
		ImGui::Separator();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Number Of Collision Pairs");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", GetNumberCollisionPairs());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Number Of Physics Objects");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", GetNumberPhysicsObjects());
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Number Of Constraints");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Text("%5.2i", static_cast<int>(m_Constraints.size()));
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Paused");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::Checkbox("##Paused", &m_IsPaused);
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Gravity");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::InputFloat3("##Gravity", &m_Gravity.x);
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Damping Factor");
		ImGui::NextColumn();
		ImGui::PushItemWidth(-1);
		ImGui::InputFloat("##Damping Factor", &m_DampingFactor);
		ImGui::PopItemWidth();
		ImGui::NextColumn();

		ImGui::AlignTextToFramePadding();
		ImGui::Text("Integration Type");
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
}
