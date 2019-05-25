#pragma once
#include "LM.h"
#include "System/String.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Shader.h"

namespace lumos
{
	namespace graphics
	{
		struct LUMOS_EXPORT RendererUniform
		{
			String uniform;
			byte* value;
		};

		struct LUMOS_EXPORT RenderCommand
		{
			Mesh* mesh;
			maths::Matrix4 transform;
			maths::Matrix4 textureMatrix;
			std::vector<RendererUniform> uniforms;
		};
	}
}