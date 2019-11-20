#pragma once

#include "lmpch.h"
#include "Utilities/TSingleton.h"
#include "PhysicsObject3D.h"
#include "Manifold.h"
#include "Broadphase.h"
#include "ECS/ISystem.h"
#include "App/Scene.h"

namespace Lumos
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
	class TimeStep;

	class LUMOS_EXPORT LumosPhysicsEngine : public ISystem
	{
	public:
		LumosPhysicsEngine();
		~LumosPhysicsEngine();

		void SetDefaults();

		//Add Constraints
		void AddConstraint(Constraint* c) { m_Constraints.push_back(c); }

		void OnInit() override {};
		//Update Physics Engine
		void OnUpdate(TimeStep* timeStep, Scene* scene) override;

		//Getters / Setters
		bool IsPaused() const { return m_IsPaused; }
		void SetPaused(bool paused) { m_IsPaused = paused; }

		const Maths::Vector3& GetGravity() const { return m_Gravity; }
		void SetGravity(const Maths::Vector3& g) { m_Gravity = g; }

		float GetDampingFactor() const { return m_DampingFactor; }
		void  SetDampingFactor(float d) { m_DampingFactor = d; }

        static float GetDeltaTime() { return s_UpdateTimestep; }

		Ref<Broadphase> GetBroadphase() const { return m_BroadphaseDetection; }
		_FORCE_INLINE_ void SetBroadphase(const Ref<Broadphase>& bp)
		{
			m_BroadphaseDetection = bp;
		}

		int GetNumberCollisionPairs() const { return static_cast<int>(m_BroadphaseCollisionPairs.size()); }
		int GetNumberPhysicsObjects() const { return static_cast<int>(m_PhysicsObjects.size()); }

		IntegrationType GetIntegrationType() const { return m_IntegrationType; }
		void SetIntegrationType(const IntegrationType& type){ m_IntegrationType = type; }

        void ClearConstraints();
        
		void OnImGui() override;
	protected:

		//The actual time-independant update function
		void UpdatePhysics(Scene* scene);

		//Handles broadphase collision detection
		void BroadPhaseCollisions();

		//Handles narrowphase collision detection
		void NarrowPhaseCollisions();

		//Updates all physics objects position, orientation, velocity etc (default method uses symplectic euler integration)
		void UpdatePhysicsObjects();
		void UpdatePhysicsObject(const Ref<PhysicsObject3D>& obj) const;

		//Solves all engine constraints (constraints and manifolds)
		void SolveConstraints();

	protected:
		bool		m_IsPaused;
		float		m_UpdateAccum;
		Maths::Vector3 m_Gravity;
		float		m_DampingFactor;

		std::vector<Ref<PhysicsObject3D>> m_PhysicsObjects;
		std::vector<CollisionPair>  m_BroadphaseCollisionPairs;

		std::vector<Constraint*>	m_Constraints;			// Misc constraints between pairs of objects
		std::vector<Manifold*>		m_Manifolds;			// Contact constraints between pairs of objects
		std::mutex					m_ManifoldsMutex;

		Ref<Broadphase> m_BroadphaseDetection;
		IntegrationType m_IntegrationType;

		bool m_MultipleUpdates = true;
        static float s_UpdateTimestep;
	};
}
