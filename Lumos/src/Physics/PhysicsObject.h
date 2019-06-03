#pragma once
#include "LM.h"
#include "Maths/Maths.h"

namespace Lumos
{

	enum class LUMOS_EXPORT Shape
	{
		Square,
		Circle,
		Custom
	};

	struct LUMOS_EXPORT PhysicsObjectParamaters
	{
		PhysicsObjectParamaters()
		{
			mass = 1.0f;
			shape = Shape::Square;
			position = Maths::Vector3(0.0f);
			scale = Maths::Vector3(1.0f);
			isStatic = false;
		}

		float mass;
		Maths::Vector3 position;
		Maths::Vector3 scale;
		bool isStatic;
		Shape shape;
	};

	class LUMOS_EXPORT PhysicsObject
	{

	public:
		PhysicsObject();

		virtual ~PhysicsObject();

		bool GetIsStatic()    const { return m_Static; }
		bool GetIsAtRest()    const { return m_AtRest; }
		float GetElasticity() const { return m_Elasticity; }
		float GetFriction()   const { return m_Friction; }
		void SetIsStatic(const bool isStatic)   { m_Static = isStatic; }

		void SetElasticity(const float elasticity) { m_Elasticity = elasticity; }
		void SetFriction(const float friction)   { m_Friction = friction; }

		virtual	void SetIsAtRest(const bool isAtRest)   { m_AtRest = isAtRest; }
		virtual void WakeUp() { m_AtRest = false; }
		bool IsAwake() const { return !m_AtRest; }

	protected:

		bool m_Static;
		float m_Elasticity;
		float m_Friction;
		bool m_AtRest;
	};

}
