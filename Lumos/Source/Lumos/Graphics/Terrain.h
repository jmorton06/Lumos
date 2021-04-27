#pragma once
#include "Mesh.h"

namespace Lumos
{
    class LUMOS_EXPORT Terrain : public Graphics::Mesh
    {
    public:
        Terrain(int width = 500, int height = 500, int lowside = 50, int lowscale = 10, float xRand = 1.0f, float yRand = 150.0f, float zRand = 1.0f, float texRandX = 1.0f / 16.0f, float texRandZ = 1.0f / 16.0f);
    };
}
