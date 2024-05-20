#pragma once
#include <glm/ext/matrix_float4x4.hpp>

namespace Lumos
{
    class TextureMatrixComponent
    {
    public:
        explicit TextureMatrixComponent(const glm::mat4& matrix = glm::mat4(1.0f));

        void OnImGui();

        const glm::mat4& GetMatrix() const { return m_TextureMatrix; }
        glm::mat4& GetMatrix() { return m_TextureMatrix; }

    private:
        glm::mat4 m_TextureMatrix;
    };
}
