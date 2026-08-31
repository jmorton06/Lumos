#pragma once

#define SceneSerialisationVersion 37 // 37: ParticleEmitter local-space flag (36: material emissive Vec4)
#include <cereal/cereal.hpp>

namespace Serialisation
{
    static int CurrentSceneVersion = 0;
}
