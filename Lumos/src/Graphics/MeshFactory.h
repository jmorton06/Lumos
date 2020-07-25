#pragma once

#include "lmpch.h"

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
		enum class PrimitiveType
		{
			Plane,
			Quad,
			Cube,
			Pyramid,
			Sphere,
			Capsule,
			Cylinder,
            None
		};

		class Mesh;

		LUMOS_EXPORT Mesh* CreatePrimative(PrimitiveType type);

		Mesh* CreateQuad();
		Mesh* CreateQuad(float x, float y, float width, float height);
		Mesh* CreateQuad(const Maths::Vector2 &position, const Maths::Vector2 &size);
		Mesh* CreateCube();
		Mesh* CreatePyramid();
		Mesh* CreateSphere(u32 xSegments = 64, u32 ySegments = 64);
		Mesh* CreateIcoSphere(u32 radius = 1, u32 subdivision = 64);
		Mesh* CreateCapsule(float radius = 1.0f, float midHeight = 1.0f, int radialSegments = 64, int rings = 8);
		Mesh* CreatePlane(float width, float height, const Maths::Vector3 &normal);
		Mesh* CreateCylinder(float bottomRadius = 1.0f, float topRadius = 1.0f, float height = 1.0f, int radialSegments = 64, int rings = 8);
	}
}
