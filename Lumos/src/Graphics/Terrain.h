#pragma once

#include "LM.h"
#include "Mesh.h"

namespace lumos
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

	class LUMOS_EXPORT Terrain : public graphics::Mesh
	{
	public:
		Terrain();
	};
}

