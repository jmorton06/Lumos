#pragma once

#include "Graphics/API/RenderPass.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderPass : public RenderPass
        {
        public:
            GLRenderPass();
            ~GLRenderPass();

            bool Init(const RenderpassInfo& renderpassCI) override;
            void BeginRenderpass(CommandBuffer * commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height, bool beginCommandBuffer = true) const override;
			void EndRenderpass(CommandBuffer* commandBuffer, bool endCommandBuffer = true) override;
            static void MakeDefault();
        protected:
            static RenderPass* CreateFuncGL();
		private:
			bool m_Clear = true;
        };
    }
}
