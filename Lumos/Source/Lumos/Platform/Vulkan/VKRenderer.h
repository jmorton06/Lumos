#pragma once

#include "VK.h"
#include "VKContext.h"
#include "VKSwapchain.h"
#include "Graphics/API/DescriptorSet.h"
#include "Graphics/API/RenderPass.h"
#include "VKUniformBuffer.h"
#include "VKDescriptorSet.h"
#include "Graphics/API/Renderer.h"

#define NUM_SEMAPHORES 10

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class LUMOS_EXPORT VKRenderer : public Renderer
        {
        public:
            VKRenderer(uint32_t width, uint32_t height)
            {
                m_Width = width;
                m_Height = height;
            }
            ~VKRenderer();

            static VKRenderer* GetRenderer()
            {
                return static_cast<VKRenderer*>(s_Instance);
            }

            Swapchain* GetSwapchainInternal() const override;
            void InitInternal() override;
            void Begin() override;
            void OnResize(uint32_t width, uint32_t height) override;

            void PresentInternal() override;
            void PresentInternal(CommandBuffer* cmdBuffer) override;

            void ClearRenderTarget(Graphics::Texture* texture, Graphics::CommandBuffer* cmdBuffer) override;
            void ClearSwapchainImage() const;

            const std::string& GetTitleInternal() const override;

            void BindDescriptorSetsInternal(Graphics::Pipeline* pipeline, Graphics::CommandBuffer* cmdBuffer, uint32_t dynamicOffset, std::vector<Graphics::DescriptorSet*>& descriptorSets) override;
            void DrawIndexedInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start) const override;
            void DrawInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, DataType datayType, void* indices) const override;

            static void MakeDefault();

        protected:
            static Renderer* CreateFuncVulkan(uint32_t width, uint32_t height);

        private:
            Lumos::Graphics::VKContext* m_Context;

            uint32_t m_CurrentSemaphoreIndex = 0;

            std::string m_RendererTitle;
            uint32_t m_Width, m_Height;

            VkDescriptorSet m_DescriptorSetPool[16];
        };
    }
}
