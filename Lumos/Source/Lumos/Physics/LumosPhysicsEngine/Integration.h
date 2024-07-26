#pragma once

#include "Maths/Vector3.h"

namespace Lumos
{

    class LUMOS_EXPORT Integration
    {
    public:
        struct State
        {
            Vec3 position;
            Vec3 velocity;
            Vec3 acceleration;
        };

        struct Derivative
        {
            Vec3 acceleration;
            Vec3 velocity;
        };

    public:
        static void RK2(State& state, float t, float dt);
        static void RK4(State& state, float t, float dt);

        static Derivative Evaluate(State& initial, float dt, float t, const Derivative& derivative);
    };
}
