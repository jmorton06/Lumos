#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "Integration.h"

namespace Lumos
{

    void Integration::RK2(State& state, float t, float dt)
    {
        const Derivative a = Evaluate(state, t, 0.0f, Derivative());
        const Derivative b = Evaluate(state, t, dt * 0.5f, a);

        const glm::vec3 dxdt = (a.velocity + b.velocity) * 0.5f;
        const glm::vec3 dvdt = (a.acceleration + b.acceleration) * 0.5f;

        state.position += dxdt * dt;
        state.velocity += dvdt * dt;
    }

    void Integration::RK4(State& state, float t, float dt)
    {
        const Derivative a = Evaluate(state, t, 0.0f, Derivative());
        const Derivative b = Evaluate(state, t, dt * 0.5f, a);
        const Derivative c = Evaluate(state, t, dt * 0.5f, b);
        const Derivative d = Evaluate(state, t, dt, c);

        const glm::vec3 dxdt = (a.velocity + (b.velocity + c.velocity) * 2.0f + d.velocity) * 1.0f / 6.0f;
        const glm::vec3 dvdt = (a.acceleration + (b.acceleration + c.acceleration) * 2.0f + d.acceleration) * 1.0f / 6.0f;

        state.position += dxdt * dt;
        state.velocity += dvdt * dt;
    }

    Integration::Derivative Integration::Evaluate(State& initial, float dt, float t, const Derivative& derivative)
    {
        State state;
        state.position = initial.position + derivative.velocity * dt;
        state.velocity = initial.velocity + derivative.acceleration * dt;

        Derivative output;
        output.velocity     = state.velocity;
        output.acceleration = initial.acceleration;
        return output;
    }

}
