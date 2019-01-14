#pragma once
#include "LM.h"
#include "System/String.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Shader.h"

namespace Lumos
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
		Shader* shader;
		std::vector<RendererUniform> uniforms;
	};
}