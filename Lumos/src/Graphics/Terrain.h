#pragma once

#include "JM.h"
#include "Mesh.h"

namespace jm
{

#define RAW_WIDTH_RANDOM     500
#define RAW_HEIGHT_RANDOM     500
#define RAW_LOWSIDE_RANDOM    50
#define RAW_LOWSCALE_RANDOM   10

#define HEIGHTMAP_X_RANDOM    1.0f
#define HEIGHTMAP_Z_RANDOM    1.0f
#define HEIGHTMAP_Y_RANDOM    150.0f
#define HEIGHTMAP_TEX_X_RANDOM   1.0f / 16.0f
#define HEIGHTMAP_TEX_Z_RANDOM   1.0f / 16.0f

	class JM_EXPORT Terrain : public Mesh
	{
	public:
		Terrain();
	};
}

