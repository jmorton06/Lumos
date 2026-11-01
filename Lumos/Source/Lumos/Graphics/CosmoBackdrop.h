#pragma once
#include "Maths/Vector3.h"

namespace Lumos
{
    namespace Graphics
    {
        // Procedural sky for space scenes. A Region describes one whole sky: it is baked once
        // into a picture, and a new one fades in over the last. Defaults = the sky seen from Sol.
        namespace CosmoBackdrop
        {
            struct Region
            {
                float Seed          = 0.0f;
                Vec3 Offset         = Vec3(0.0f); // moves the noise, so no two skies share clouds
                float Nebula        = 0.35f;
                float Band          = 0.65f;
                float Scale         = 1.0f;
                float Wander        = 0.10f; // how uneven the band's edges are
                float Tightness     = 42.0f; // bigger = thinner band
                float Bulge         = 1.0f;
                float Lanes         = 0.85f; // dark dust down the plane
                float Density       = 0.0f;  // nebulae: + = more of the sky
                Vec3 Pole           = Vec3(-0.8676f, -0.1980f, 0.4560f);  // north galactic pole, J2000
                Vec3 Centre         = Vec3(-0.0546f, -0.8728f, -0.4851f); // galactic centre
                Vec3 ColourA        = Vec3(0.42f, 0.16f, 0.58f);
                Vec3 ColourB        = Vec3(0.10f, 0.34f, 0.58f);
                Vec3 ColourC        = Vec3(0.62f, 0.22f, 0.12f);
                Vec3 Tint           = Vec3(1.0f);
                float FadeSeconds   = 3.0f;
            };

            struct State
            {
                bool Enabled = false;
                float Gain   = 1.0f; // applied live, no re-bake
                Region Sky;
                uint32_t Version = 1; // bump after changing Sky
            };

            State& Get();
        }
    }
}
