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
            void Unload() const  override;
            void BeginRenderpass(CommandBuffer * commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame,
                                 SubPassContents contents, uint32_t width, uint32_t height) const  override;
            void EndRenderpass(CommandBuffer* commandBuffer) override;

            vk::RenderPass GetRenderpass() const { return m_RenderPass; };
            static void MakeDefault();
        protected:
            static RenderPass* CreateFuncVulkan();
		private:
			vk::RenderPass 	m_RenderPass;
			vk::ClearValue* m_ClearValue;
			int 			m_ClearCount;
			bool 			m_DepthOnly;
			bool			m_ClearDepth;
		};
	}
}
