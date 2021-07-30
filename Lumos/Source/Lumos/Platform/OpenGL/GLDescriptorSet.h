#pragma once

#include "Graphics/RHI/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLShader;
        class GLDescriptorSet : public DescriptorSet
        {
        public:
            GLDescriptorSet(const DescriptorDesc& info);

            ~GLDescriptorSet() {};

            void Update() override;
            void SetTexture(const std::string& name, Texture* texture, TextureType textureType) override;
            void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType) override;
            void SetBuffer(const std::string& name, UniformBuffer* buffer) override;

            void Bind(uint32_t offset = 0);

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
            static void MakeDefault();

        protected:
            static DescriptorSet* CreateFuncGL(const DescriptorDesc& info);

        private:
            uint32_t m_DynamicOffset = 0;
            GLShader* m_Shader = nullptr;

            std::vector<Descriptor> m_Descriptors;
        };
    }
}
