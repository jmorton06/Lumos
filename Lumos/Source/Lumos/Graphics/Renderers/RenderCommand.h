#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/RHI/Shader.h"

namespace Lumos
{

    namespace Graphics
    {
        class Material;

        struct LUMOS_EXPORT RenderCommand
        {
            Mesh* mesh = nullptr;
            Material* material = nullptr;
            Maths::Matrix4 transform;
            Maths::Matrix4 textureMatrix;
            bool animated = false;
        };
    }
}
