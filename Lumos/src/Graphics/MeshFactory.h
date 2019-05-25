#pragma once

#include "LM.h"

namespace lumos
{
    class Material;

    namespace maths
    {
        class Vector2;
        class Vector3;
    }

	namespace graphics
	{
		class Mesh;

		LUMOS_EXPORT Mesh* CreateQuad();
		LUMOS_EXPORT Mesh* CreateQuad(float x, float y, float width, float height, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateQuad(const maths::Vector2 &position, const maths::Vector2 &size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateCube(float size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreatePyramid(float size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateSphere(uint xSegments, uint ySegments, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateIcoSphere(uint radius, uint subdivision, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreatePlane(float width, float height, const maths::Vector3 &normal, std::shared_ptr<Material> material);
	}
}
