#pragma once
#include "LM.h"
#include "Core/String.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{
	class Material;

	namespace Graphics
	{
		struct LUMOS_EXPORT RendererUniform
		{
			String uniform;
			u8* value;
		};

		struct LUMOS_EXPORT RenderCommand
		{
			Mesh* mesh;
			Material* material;
			Maths::Matrix4 transform;
			Maths::Matrix4 textureMatrix;
			std::vector<RendererUniform> uniforms;
		};
	}
}
