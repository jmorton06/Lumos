#pragma once
#include "Maths/Matrix4.h"

namespace Lumos
{
    class TextureMatrixComponent
    {
    public:
        explicit TextureMatrixComponent(const Maths::Matrix4& matrix = Maths::Matrix4());

        void OnImGui();

        const Maths::Matrix4& GetMatrix() const { return m_TextureMatrix; }
        Maths::Matrix4& GetMatrix() { return m_TextureMatrix; }

    private:
        Maths::Matrix4 m_TextureMatrix;
    };
}
