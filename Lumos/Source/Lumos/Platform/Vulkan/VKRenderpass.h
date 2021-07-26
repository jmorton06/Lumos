#pragma once

#include "VKDevice.h"
#include "Graphics/RHI/RenderPass.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKRenderpass : public RenderPass
        {
        public:
            VKRenderpass(const RenderPassDesc& renderPassCI);
            ~VKRenderpass();

            bool Init(const RenderPassDesc& renderpassCI);
            void BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const override;
            void EndRenderpass(CommandBuffer* commandBuffer) override;

            const VkRenderPass& GetHandle() const { return m_RenderPass; };
            int GetAttachmentCount() const override { return m_ClearCount; };
            int GetColourAttachmentCount() const { return m_ColourAttachmentCount; }

            static void MakeDefault();

        protected:
            static RenderPass* CreateFuncVulkan(const RenderPassDesc& renderPassCI);

        private:
            VkRenderPass m_RenderPass;
            VkClearValue* m_ClearValue;
            int m_ClearCount;
            int m_ColourAttachmentCount;
            bool m_DepthOnly;
            bool m_ClearDepth;
        };
    }
}
