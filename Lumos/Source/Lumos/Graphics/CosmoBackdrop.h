#pragma once
#include "Maths/Vector3.h"

namespace Lumos
{
    namespace Graphics
    {
        namespace CosmoBackdrop
        {
            struct State
            {
                bool Enabled          = false;
                float NebulaIntensity = 0.35f;
                float BandIntensity   = 0.5f;
                float Scale           = 1.0f;
                float Seed            = 0.0f;
                Vec3 CamPos           = Vec3(0.0f, 0.0f, 0.0f); // pre-scaled by the game
            };

            State& Get();
        }
    }
}
