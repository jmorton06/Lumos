#pragma once
#include "Maths/Matrix4.h"

namespace Lumos
{
    namespace Graphics
    {
        class Mesh;
        class Material;
        class Pipeline;
        class DescriptorSet;

        struct LUMOS_EXPORT RenderCommand
        {
            Mesh* mesh         = nullptr;
            Material* material = nullptr;
            Pipeline* pipeline = nullptr;
            Mat4 transform;
            Mat4 textureMatrix;
            bool animated                        = false;
            DescriptorSet* AnimatedDescriptorSet = nullptr;
            u32 instanceCount                    = 1;
            u32 firstInstance                    = 0;
            bool instanced                       = false;
        };
    }
}
