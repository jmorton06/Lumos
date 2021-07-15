#pragma once

#include "Graphics/RHI/RenderPass.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLRenderPass : public RenderPass
        {
        public:
            GLRenderPass(const RenderPassDesc& renderpassCI);
            ~GLRenderPass();

            bool Init(const RenderPassDesc& renderpassCI);
            void BeginRenderpass(CommandBuffer* commandBuffer, const Maths::Vector4& clearColour, Framebuffer* frame, SubPassContents contents, uint32_t width, uint32_t height) const override;
            void EndRenderpass(CommandBuffer* commandBuffer) override;
            int GetAttachmentCount() const override { return m_ClearCount; };

            static void MakeDefault();

        protected:
            static RenderPass* CreateFuncGL(const RenderPassDesc& renderpassCI);

        private:
            bool m_Clear = true;
            int m_ClearCount = 0;
        };
    }
}
