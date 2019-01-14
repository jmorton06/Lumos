#include "LM.h"
#include "JMPhysicsEngine.h"
#include "CollisionDetection.h"
#include "PhysicsObject3D.h"
#include "App/Window.h"

#include "Integration.h"
#include "Constraint.h"
#include "Entity/Entity.h"
#include "Utilities/TimeStep.h"

namespace Lumos
{

	JMPhysicsEngine::JMPhysicsEngine()
		: m_IsPaused(true)
		, m_UpdateTimestep(1.0f / 60.f)
		, m_UpdateAccum(0.0f)
		, m_Gravity(maths::Vector3(0.0f, -9.81f, 0.0f))
		, m_DampingFactor(0.999f)
		, m_BroadphaseDetection(nullptr)
		, m_integrationType(IntegrationType::INTEGRATION_RUNGE_KUTTA_4)
	{
	}

	void JMPhysicsEngine::SetDefaults()
	{
		m_IsPaused = true;
		m_UpdateTimestep = 1.0f / 60.f;
		m_UpdateAccum = 0.0f;
		m_Gravity = maths::Vector3(0.0f, -9.81f, 0.0f);
		m_DampingFactor = 0.999f;
		m_integrationType = IntegrationType::INTEGRATION_RUNGE_KUTTA_4;
	}

	JMPhysicsEngine::~JMPhysicsEngine()
	{
		RemoveAllPhysicsObjects();

		CollisionDetection::Release();

		if (m_BroadphaseDetection != nullptr)
			delete m_BroadphaseDetection;
	}

	void JMPhysicsEngine::AddPhysicsObject(std::shared_ptr<PhysicsObject3D> obj)
	{
		m_PhysicsObjects.push_back(obj);
	}

	void JMPhysicsEngine::RemovePhysicsObject(std::shared_ptr<PhysicsObject3D> obj)
	{
		//// Lookup the object in question
		const auto it = std::find(m_PhysicsObjects.begin(), m_PhysicsObjects.end(), obj);

		// If found, remove it from the list
		if (it != m_PhysicsObjects.end())
			m_PhysicsObjects.erase(it);
	}

	void JMPhysicsEngine::RemoveAllPhysicsObjects()
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

	void JMPhysicsEngine::Update(TimeStep* timeStep)
	{
		if (!m_IsPaused)
		{
			m_UpdateTimestep = timeStep->GetSeconds();

			UpdatePhysics();
		}
	}

	void JMPhysicsEngine::UpdatePhysics()
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

	void JMPhysicsEngine::DebugRender(uint64 debugFlags)
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

	void JMPhysicsEngine::UpdatePhysicsObjects()
	{
		for (const auto& obj : m_PhysicsObjects)
		{
			UpdatePhysicsObject(obj.get());
		}
	}

	void JMPhysicsEngine::UpdatePhysicsObject(PhysicsObject3D* obj) const
	{
		if (!obj->GetIsStatic() && obj->IsAwake())
		{
			const float damping = m_DampingFactor;

			// Apply gravity
			if (obj->m_InvMass > 0.0f)
				obj->m_LinearVelocity += m_Gravity * m_UpdateTimestep;

			switch (m_integrationType)
			{
			case INTEGRATION_EXPLICIT_EULER:
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
			case INTEGRATION_SEMI_IMPLICIT_EULER:
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

			case INTEGRATION_RUNGE_KUTTA_2:
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

			case INTEGRATION_RUNGE_KUTTA_4:
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

	void JMPhysicsEngine::BroadPhaseCollisions()
	{
		m_BroadphaseCollisionPairs.clear();
		if (m_BroadphaseDetection)
			m_BroadphaseDetection->FindPotentialCollisionPairs(m_PhysicsObjects, m_BroadphaseCollisionPairs);
	}

	void JMPhysicsEngine::NarrowPhaseCollisions()
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

	void JMPhysicsEngine::SolveConstraints()
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
	PhysicsObject3D* JMPhysicsEngine::FindObjectByName(const String& name)
	{
		auto it = std::find_if(m_PhysicsObjects.begin(), m_PhysicsObjects.end(), [name](std::shared_ptr<PhysicsObject3D> o) 
		{
			Entity *po = o->GetAssociatedObject();
			return (po != nullptr && po->GetName() == name);
		});

		return (it == m_PhysicsObjects.end()) ? nullptr : (*it).get();
	}
}