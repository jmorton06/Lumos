#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "TextureMatrixComponent.h"

namespace Lumos
{
    TextureMatrixComponent::TextureMatrixComponent(const glm::mat4& matrix)
        : m_TextureMatrix(matrix)
    {
    }

    void TextureMatrixComponent::OnImGui()
    {
    }
}
