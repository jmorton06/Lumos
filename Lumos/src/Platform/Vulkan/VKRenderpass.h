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
            VKRenderpass();
            ~VKRenderpass();

            bool Init(const RenderpassInfo& renderpassCI) override;
            void BeginRenderpass(CommandBuffer * commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height, bool beginCommandBuffer = true) const override;
            void EndRenderpass(CommandBuffer* commandBuffer, bool endCommandBuffer = true) override;

            const VkRenderPass& GetRenderpass() const { return m_RenderPass; };
            int GetAttachmentCount() const override { return m_ClearCount; };

            static void MakeDefault();
        protected:
            static RenderPass* CreateFuncVulkan();
		private:
			VkRenderPass 	m_RenderPass;
			VkClearValue*   m_ClearValue;
			int 			m_ClearCount;
			bool 			m_DepthOnly;
			bool			m_ClearDepth;
            
		};
	}
}
