#pragma once

#include "VKDevice.h"
#include "Graphics/API/RenderPass.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKRenderpass : public RenderPass
		{
		public:
            VKRenderpass(const RenderPassInfo& renderPassCI);
            ~VKRenderpass();

            bool Init(const RenderPassInfo& renderpassCI);
            void BeginRenderpass(CommandBuffer * commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height, bool beginCommandBuffer = false) const override;
            void EndRenderpass(CommandBuffer* commandBuffer, bool endCommandBuffer = false) override;

            const VkRenderPass& GetRenderpass() const { return m_RenderPass; };
            int GetAttachmentCount() const override { return m_ClearCount; };

            static void MakeDefault();
        protected:
            static RenderPass* CreateFuncVulkan(const RenderPassInfo& renderPassCI);
		private:
			VkRenderPass 	m_RenderPass;
			VkClearValue*   m_ClearValue;
			int 			m_ClearCount;
			bool 			m_DepthOnly;
			bool			m_ClearDepth;
            
		};
	}
}
