#pragma once
#include "LM.h"
#include "Graphics/API/RenderPass.h"

namespace Lumos
{
    namespace graphics
    {
        class GLRenderPass : public api::RenderPass
        {

        public:
            GLRenderPass();
            ~GLRenderPass();

            bool Init(const api::RenderpassInfo& renderpassCI) override;
            void Unload() const  override;
            void BeginRenderpass(api::CommandBuffer * commandBuffer, const maths::Vector4& clearColour, Framebuffer* frame,
                                 api::SubPassContents contents, uint32_t width, uint32_t height) const  override;
            void EndRenderpass(api::CommandBuffer* commandBuffer) override;
        };
    }
}