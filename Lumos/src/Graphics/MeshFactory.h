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
		LUMOS_EXPORT Mesh* CreateQuad(float x, float y, float width, float height, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateQuad(const Maths::Vector2 &position, const Maths::Vector2 &size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateCube(float size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreatePyramid(float size, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateSphere(uint xSegments, uint ySegments, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreateIcoSphere(uint radius, uint subdivision, std::shared_ptr<Material> material);
		LUMOS_EXPORT Mesh* CreatePlane(float width, float height, const Maths::Vector3 &normal, std::shared_ptr<Material> material);
	}
}
