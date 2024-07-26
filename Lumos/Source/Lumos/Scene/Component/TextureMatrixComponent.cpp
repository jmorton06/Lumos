#include "Precompiled.h"
#include "TextureMatrixComponent.h"

namespace Lumos
{
    TextureMatrixComponent::TextureMatrixComponent(const Mat4& matrix)
        : m_TextureMatrix(matrix)
    {
    }

    void TextureMatrixComponent::OnImGui()
    {
    }
}
