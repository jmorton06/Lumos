#include "Precompiled.h"
#include "TextureMatrixComponent.h"

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
