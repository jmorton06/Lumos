#include "Precompiled.h"
#include "Integration.h"

namespace Lumos
{

    void Integration::RK2(State& state, float t, float dt)
    {
        State c, d;

        const Derivative a = Evaluate(state, t, 0.0f, Derivative());
        const Derivative b = Evaluate(state, t, dt * 0.5f, a);

        const Maths::Vector3 dxdt = (a.velocity + b.velocity) * 0.5f;
        const Maths::Vector3 dvdt = (a.acceleration + b.acceleration) * 0.5f;

        state.position += dxdt * dt;
        state.velocity += dvdt * dt;
    }

    void Integration::RK4(State& state, float t, float dt)
    {
        const Derivative a = Evaluate(state, t, 0.0f, Derivative());
        const Derivative b = Evaluate(state, t, dt * 0.5f, a);
        const Derivative c = Evaluate(state, t, dt * 0.5f, b);
        const Derivative d = Evaluate(state, t, dt, c);

        //const Vector3 dxdt = (a.velocity + (b.velocity + c.velocity) * 2.0f + d.velocity) * 1.0f / 6.0f;
        //const Vector3 dvdt = (a.acceleration + (b.acceleration + c.acceleration) * 2.0f + d.acceleration) * 1.0f / 6.0f;

        const Maths::Vector3 dxdt = (a.velocity + (b.velocity + c.velocity) * 2.0f + d.velocity) * 1.0f / 6.0f;
        const Maths::Vector3 dvdt = (a.acceleration + (b.acceleration + c.acceleration) * 2.0f + d.acceleration) * 1.0f / 6.0f;

        state.position += dxdt * dt;
        state.velocity += dvdt * dt;
    }

    Integration::Derivative Integration::Evaluate(State& initial, float dt, float t, const Derivative& derivative)
    {
        /*initial.position += derivative.velocity * dt;
		initial.velocity += derivative.acceleration * dt;

		State out;
		out.velocity = initial.velocity;
		out.acceleration = initial.acceleration;

		return out;*/

        State state;
        state.position = initial.position + derivative.velocity * dt;
        state.velocity = initial.velocity + derivative.acceleration * dt;

        Derivative output;
        output.velocity = state.velocity;
        output.acceleration = initial.acceleration;
        return output;
    }

}