#pragma once

#include "VKDevice.h"
#include "Graphics/API/RenderPass.h"

namespace Lumos
{
	namespace graphics
	{
		class VKRenderpass : public api::RenderPass
		{
		public:
				VKRenderpass();
				~VKRenderpass();

				bool Init(const api::RenderpassInfo& renderpassCI) override;
				void Unload() const  override;
				void BeginRenderpass(api::CommandBuffer * commandBuffer, const maths::Vector4& clearColour, Framebuffer* frame,
				                     api::SubPassContents contents, uint32_t width, uint32_t height) const  override;
				void EndRenderpass(api::CommandBuffer* commandBuffer) override;

				vk::RenderPass GetRenderpass() const { return m_RenderPass; };

		private:
			vk::RenderPass 	m_RenderPass;
			vk::ClearValue* m_ClearValue;
			int 			m_ClearCount;
			bool 			m_DepthOnly;
			bool			m_ClearDepth;
		};
	}
}
