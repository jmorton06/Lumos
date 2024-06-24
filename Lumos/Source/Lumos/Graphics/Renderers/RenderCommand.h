#pragma once

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
            glm::mat4 transform;
            glm::mat4 textureMatrix;
            bool animated                        = false;
            DescriptorSet* AnimatedDescriptorSet = nullptr;
        };
    }
}
