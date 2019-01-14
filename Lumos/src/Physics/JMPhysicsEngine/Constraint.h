#pragma once
#include "JM.h"

namespace jm
{

	class JM_EXPORT Constraint
	{
	public:
		virtual ~Constraint() = default;

		Constraint()
		{
		}

		virtual void ApplyImpulse() = 0;

		virtual void PreSolverStep(float dt)
		{
		}

		virtual void DebugDraw() const
		{
		}
	};
}
