#pragma once

#include "Maths/Maths.h"

namespace Lumos
{

    class LUMOS_EXPORT Integration
    {
    public:
        struct State
        {
            Maths::Vector3 position;
            Maths::Vector3 velocity;
            Maths::Vector3 acceleration;
        };

        struct Derivative
        {
            Maths::Vector3 acceleration;
            Maths::Vector3 velocity;
        };

    public:
        static void RK2(State& state, float t, float dt);
        static void RK4(State& state, float t, float dt);

        static Derivative Evaluate(State& initial, float dt, float t, const Derivative& derivative);
    };
}
