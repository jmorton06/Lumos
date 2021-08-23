#pragma once
#include "Graphics/RHI/DescriptorSet.h"
#include "VK.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Core/Buffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKDescriptorSet : public DescriptorSet
        {
        public:
            VKDescriptorSet(const DescriptorDesc& descriptorDesc);
            ~VKDescriptorSet();

            VkDescriptorSet GetDescriptorSet()
            {
                uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
                return m_DescriptorSet[currentFrame];
            }
            void Update() override;
            void SetTexture(const std::string& name, Texture* texture, TextureType textureType) override;
            void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType) override;
            void SetBuffer(const std::string& name, UniformBuffer* buffer) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) override;
            void SetUniformBufferData(const std::string& bufferName, void* data) override;

            Graphics::UniformBuffer* GetUnifromBuffer(const std::string& name) override;
            bool GetIsDynamic() const { return m_Dynamic; }

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }

            static void MakeDefault();

        protected:
            void UpdateInternal(std::vector<Descriptor>* imageInfos);

            static DescriptorSet* CreateFuncVulkan(const DescriptorDesc&);

        private:
            uint32_t m_DynamicOffset = 0;
            Shader* m_Shader = nullptr;
            bool m_Dynamic = false;
            VkDescriptorBufferInfo* m_BufferInfoPool = nullptr;
            VkDescriptorImageInfo* m_ImageInfoPool = nullptr;
            VkWriteDescriptorSet* m_WriteDescriptorSetPool = nullptr;

            uint32_t m_FramesInFlight = 0;

            struct UniformBufferInfo
            {
                SharedPtr<UniformBuffer> UB;
                std::vector<BufferMemberInfo> m_Members;
                Buffer LocalStorage;
                bool HasUpdated;
            };

            std::map<uint32_t, VkDescriptorSet> m_DescriptorSet;
            std::map<uint32_t, DescriptorSetInfo> m_Descriptors;
            std::map<uint32_t, std::map<std::string, UniformBufferInfo>> m_UniformBuffers;
        };
    }
}
