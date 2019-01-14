#pragma once
#include "JM.h"
#include "System/String.h"
#include "Graphics/Mesh.h"
#include "Graphics/API/Shader.h"

namespace jm
{

	struct JM_EXPORT RendererUniform
	{
		String uniform;
		byte* value;
	};

	struct JM_EXPORT RenderCommand
	{
		Mesh* mesh;
		maths::Matrix4 transform;
		maths::Matrix4 textureMatrix;
		Shader* shader;
		std::vector<RendererUniform> uniforms;
	};
}