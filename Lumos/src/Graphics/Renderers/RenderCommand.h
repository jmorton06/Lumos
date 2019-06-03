#pragma once
#include "LM.h"
#include "System/String.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{
	namespace Graphics
	{
		struct LUMOS_EXPORT RendererUniform
		{
			String uniform;
			byte* value;
		};

		struct LUMOS_EXPORT RenderCommand
		{
			Mesh* mesh;
			Maths::Matrix4 transform;
			Maths::Matrix4 textureMatrix;
			std::vector<RendererUniform> uniforms;
		};
	}
}