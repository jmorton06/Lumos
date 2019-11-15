﻿#include "lmpch.h"
#include "TextureMatrixComponent.h"
#include <imgui/imgui.h>

namespace Lumos
{
	TextureMatrixComponent::TextureMatrixComponent(const Maths::Matrix4& matrix)
		: m_TextureMatrix(matrix)
	{
	}

	void TextureMatrixComponent::OnImGui()
	{
	}
}
