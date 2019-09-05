#pragma once

#include "LM.h"

namespace Lumos
{
    class Material;

    namespace Maths
    {
        class Vector2;
        class Vector3;
    }

	namespace Graphics
	{
		class Mesh;

		LUMOS_EXPORT Mesh* CreateQuad();
		LUMOS_EXPORT Mesh* CreateQuad(float x, float y, float width, float height);
		LUMOS_EXPORT Mesh* CreateQuad(const Maths::Vector2 &position, const Maths::Vector2 &size);
		LUMOS_EXPORT Mesh* CreateCube(float size);
		LUMOS_EXPORT Mesh* CreatePyramid(float size);
		LUMOS_EXPORT Mesh* CreateSphere(u32 xSegments, u32 ySegments);
		LUMOS_EXPORT Mesh* CreateIcoSphere(u32 radius, u32 subdivision);
		LUMOS_EXPORT Mesh* CreateCapsule(float radius = 1.0f, float midHeight = 1.0f, int radialSegments = 64, int rings = 8);
		LUMOS_EXPORT Mesh* CreatePlane(float width, float height, const Maths::Vector3 &normal);
	}
}
