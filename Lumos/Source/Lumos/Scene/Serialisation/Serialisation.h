#pragma once

#define SceneSerialisationVersion 41 // 41: spot light shadows, 40: decals, TAA / contact shadow / volumetric fog / GI settings, 39: mesh collider detail, material name + render flags, 38: adaptive exposure settings, 37: ParticleEmitter local-space flag (36: material emissive Vec4)
#include <cereal/cereal.hpp>
#include "Core/Core.h"

namespace Serialisation
{
    // Version of the scene file being loaded. One variable for the whole engine: as a
    // header static each source file had its own, and loaders compiled elsewhere read 0.
    extern LUMOS_EXPORT int CurrentSceneVersion;
}
