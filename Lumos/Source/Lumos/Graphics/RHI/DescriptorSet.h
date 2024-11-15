#pragma once
#include "RHIDefinitions.h"

namespace Lumos
{
    struct Buffer;
    namespace Graphics
    {
        class DescriptorSet
        {
        public:
            virtual ~DescriptorSet() = default;
            static DescriptorSet* Create(const DescriptorDesc& desc);

            virtual void Update(CommandBuffer* cmdBuffer = nullptr)                                                                              = 0;
            virtual void SetDynamicOffset(uint32_t offset)                                                                                       = 0;
            virtual uint32_t GetDynamicOffset() const                                                                                            = 0;
            virtual void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType = TextureType(0)) = 0;
            virtual void SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex = 0, TextureType textureType = TextureType(0))  = 0;
            virtual void SetBuffer(const std::string& name, UniformBuffer* buffer)                                                               = 0;
            virtual Graphics::UniformBuffer* GetUniformBuffer(const std::string& name)                                                           = 0;
            virtual void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data)                                   = 0;
            virtual void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size)                    = 0;
            virtual void SetUniformBufferData(const std::string& bufferName, void* data)                                                         = 0;

            virtual void SetTexture(u8 binding, Texture* texture, uint32_t mipIndex = 0, TextureType textureType = TextureType(0)) = 0;
            virtual void SetTexture(u8 binding, Texture** texture, uint32_t textureCount, TextureType textureType = TextureType(0)) = 0;
            virtual void SetBuffer(u8 binding, UniformBuffer* buffer) = 0;
            virtual void SetUniform(u8 binding, const std::string& uniformName, void* data) = 0;
            virtual void SetUniform(u8 binding, const std::string& uniformName, void* data, uint32_t size) = 0;
            virtual void SetUniformBufferData(u8 binding, void* data) = 0;
            virtual void SetUniformBufferData(u8 binding, void* data, float size) = 0;

            virtual void TransitionImages(CommandBuffer* commandBuffer = nullptr) { }
            virtual void SetUniformDynamic(const std::string& bufferName, uint32_t size) { }
            virtual Buffer* GetUniformBufferLocalData(const std::string& name) { return nullptr; }

            virtual Buffer* GetUniformBufferLocalData(u8 binding) { return nullptr; }
            virtual Graphics::UniformBuffer* GetUniformBuffer(u8 binding) = 0;


        protected:
            static DescriptorSet* (*CreateFunc)(const DescriptorDesc&);
        };
    }
}
