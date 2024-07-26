#pragma once
#include "Maths/Matrix4.h"

namespace Lumos
{
    class TextureMatrixComponent
    {
    public:
        explicit TextureMatrixComponent(const Mat4& matrix = Mat4(1.0f));

        void OnImGui();

        const Mat4& GetMatrix() const { return m_TextureMatrix; }
        Mat4& GetMatrix() { return m_TextureMatrix; }

    private:
        Mat4 m_TextureMatrix;
    };
}
