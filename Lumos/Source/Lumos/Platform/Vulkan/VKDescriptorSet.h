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
            void Update(CommandBuffer* cmdBuffer) override;
            void SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex, TextureType textureType) override;
            void SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType) override;
            void SetBuffer(const std::string& name, UniformBuffer* buffer) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data) override;
            void SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size) override;
            void SetUniformBufferData(const std::string& bufferName, void* data) override;
            void TransitionImages(CommandBuffer* commandBuffer) override;

            Buffer* GetUniformBufferLocalData(const std::string& name) override;
            Graphics::UniformBuffer* GetUniformBuffer(const std::string& name) override;
            bool GetIsDynamic() const { return m_Dynamic; }

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
            bool GetHasUpdated(uint32_t frame) { return m_DescriptorUpdated[frame]; }
            void SetUniformDynamic(const std::string& bufferName, uint32_t size) override;

            static void MakeDefault();

        protected:
            void UpdateInternal(TDArray<Descriptor>* imageInfos);

            static DescriptorSet* CreateFuncVulkan(const DescriptorDesc&);

        private:
            uint32_t m_DynamicOffset = 0;
            Shader* m_Shader         = nullptr;
            bool m_Dynamic           = false;

            uint32_t m_FramesInFlight = 0;

            struct UniformBufferInfo
            {
                TDArray<BufferMemberInfo> m_Members;
                Buffer LocalStorage;

                // Per frame in flight
                bool HasUpdated[MAX_FRAMES_FLIGHT];
            };

            DescriptorSetInfo m_Descriptors;
            std::map<std::string, UniformBufferInfo> m_UniformBuffersData;

            std::map<std::string, SharedPtr<UniformBuffer>> m_UniformBuffers[MAX_FRAMES_FLIGHT];
            VkDescriptorSet m_DescriptorSet[MAX_FRAMES_FLIGHT];
            VkDescriptorPool m_DescriptorPoolCreatedFrom[MAX_FRAMES_FLIGHT];
            bool m_DescriptorDirty[MAX_FRAMES_FLIGHT];
            bool m_DescriptorUpdated[MAX_FRAMES_FLIGHT];
        };
    }
}
