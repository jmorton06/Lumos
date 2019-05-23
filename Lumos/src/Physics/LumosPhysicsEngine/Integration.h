#pragma once

#include "LM.h"
#include "Maths/Maths.h"

namespace lumos
{

	class LUMOS_EXPORT Integration
	{
	public:

		struct State
		{
			maths::Vector3 position;
			maths::Vector3 velocity;
			maths::Vector3 acceleration;
		};

		struct Derivative
		{
			maths::Vector3 acceleration;
			maths::Vector3 velocity;
		};

	public:
		static void RK2(State &state, float t, float dt);
		static void RK4(State &state, float t, float dt);

		static Derivative Evaluate(State& initial, float dt, float t, const Derivative& derivative);
	};
}
