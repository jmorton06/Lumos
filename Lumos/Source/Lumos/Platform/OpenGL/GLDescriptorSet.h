#pragma once

#include "Graphics/RHI/DescriptorSet.h"
#include "Core/Buffer.h"
#include "GLUniformBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLShader;
        class GLDescriptorSet : public DescriptorSet
        {
        public:
            GLDescriptorSet(const DescriptorDesc& descriptorDesc);

            ~GLDescriptorSet() {};

            void Update() override;
            void SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex, TextureType textureType) override;
            void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType) override;
            void SetBuffer(const std::string& name, UniformBuffer* buffer) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) override;
            void SetUniformBufferData(const std::string& bufferName, void* data) override;

            Graphics::UniformBuffer* GetUnifromBuffer(const std::string& name) override;
            void Bind(uint32_t offset = 0);

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
            static void MakeDefault();

        protected:
            static DescriptorSet* CreateFuncGL(const DescriptorDesc& descriptorDesc);

        private:
            uint32_t m_DynamicOffset = 0;
            GLShader* m_Shader       = nullptr;

            std::unordered_map<std::string, uint32_t> m_UniformLocations;
            std::vector<Descriptor> m_Descriptors;
            struct UniformBufferInfo
            {
                SharedPtr<UniformBuffer> UB;
                std::vector<BufferMemberInfo> m_Members;
                Buffer LocalStorage;
                bool HasUpdated;
            };
            std::unordered_map<std::string, UniformBufferInfo> m_UniformBuffers;
        };
    }
}
