#pragma once

#include "LM.h"
#include "Utilities/TSingleton.h"
#include "PhysicsObject3D.h"
#include "Manifold.h"
#include "Broadphase.h"
#include "App/ISystem.h"

namespace lumos
{

#define SOLVER_ITERATIONS 50

	enum class LUMOS_EXPORT IntegrationType
	{
		EXPLICIT_EULER = 0,
		SEMI_IMPLICIT_EULER,
		RUNGE_KUTTA_2,
		RUNGE_KUTTA_4
	};

	class Constraint;
	struct TimeStep;

	class LUMOS_EXPORT LumosPhysicsEngine : public TSingleton<LumosPhysicsEngine> , public ISystem
	{
		friend class TSingleton<LumosPhysicsEngine>;
	public:
		LumosPhysicsEngine();
		~LumosPhysicsEngine();

		void SetDefaults();

		//Add/Remove Physics Objects
		void AddPhysicsObject(std::shared_ptr<PhysicsObject3D> obj);
		void RemovePhysicsObject(std::shared_ptr<PhysicsObject3D> obj);
		void RemoveAllPhysicsObjects(); //Delete all physics entities etc and reset-physics environment for new scene to be initialized

		//Add Constraints
		void AddConstraint(Constraint* c) { m_Constraints.push_back(c); }

		void OnInit() override {};
		//Update Physics Engine
		void OnUpdate(TimeStep* timeStep) override;			//Remember DeltaTime is 'seconds' since last update not milliseconds

		//Debug draw all physics objects, manifolds and constraints
		void DebugRender(uint64 debugFlags);

		//Getters / Setters
		bool IsPaused() const { return m_IsPaused; }
		void SetPaused(bool paused) { m_IsPaused = paused; }

		void SetUpdateTimestep(float updateTimestep) { m_UpdateTimestep = updateTimestep; }
		float GetUpdateTimestep() const { return m_UpdateTimestep; }

		const maths::Vector3& GetGravity() const { return m_Gravity; }
		void SetGravity(const maths::Vector3& g) { m_Gravity = g; }

		float GetDampingFactor() const { return m_DampingFactor; }
		void  SetDampingFactor(float d) { m_DampingFactor = d; }

		float GetDeltaTime() const { return m_UpdateTimestep; }

		Broadphase* GetBroadphase() const { return m_BroadphaseDetection; }
		inline void SetBroadphase(Broadphase* bp)
		{
			if (m_BroadphaseDetection)
				delete m_BroadphaseDetection;

			m_BroadphaseDetection = bp;
		}

		int GetNumberCollisionPairs() const { return static_cast<int>(m_BroadphaseCollisionPairs.size()); }
		int GetNumberPhysicsObjects() const { return static_cast<int>(m_PhysicsObjects.size()); }

		IntegrationType GetIntegrationType() const { return m_IntegrationType; }
		void SetIntegrationType(const IntegrationType& type){ m_IntegrationType = type; }

		PhysicsObject3D* FindObjectByName(const String& name);

		void OnIMGUI() override;
	protected:

		//The actual time-independant update function
		void UpdatePhysics();

		//Handles broadphase collision detection
		void BroadPhaseCollisions();

		//Handles narrowphase collision detection
		void NarrowPhaseCollisions();

		//Updates all physics objects position, orientation, velocity etc (default method uses symplectic euler integration)
		void UpdatePhysicsObjects();
		void UpdatePhysicsObject(PhysicsObject3D* obj) const;

		//Solves all engine constraints (constraints and manifolds)
		void SolveConstraints();

	protected:
		bool		m_IsPaused;
		float		m_UpdateTimestep, m_UpdateAccum;

		maths::Vector3 m_Gravity;
		float		m_DampingFactor;

		std::vector<std::shared_ptr<PhysicsObject3D>> m_PhysicsObjects;
		std::vector<CollisionPair>  m_BroadphaseCollisionPairs;

		std::vector<Constraint*>	m_Constraints;			// Misc constraints between pairs of objects
		std::vector<Manifold*>		m_Manifolds;			// Contact constraints between pairs of objects
		std::mutex					m_ManifoldsMutex;

		Broadphase* m_BroadphaseDetection;
		IntegrationType m_IntegrationType;

		bool m_MultipleUpdates = true;
	};
}
