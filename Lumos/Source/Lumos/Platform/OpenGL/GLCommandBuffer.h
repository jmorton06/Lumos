#pragma once

#include "Graphics/RHI/CommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLCommandBuffer : public CommandBuffer
        {
        public:
            GLCommandBuffer();
            ~GLCommandBuffer();

            bool Init(bool primary) override;
            void Unload() override;
            void BeginRecording() override;
            void BeginRecordingSecondary(RenderPass* renderPass, Framebuffer* framebuffer) override;
            void EndRecording() override;
            void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;

            void BindPipeline(Pipeline* pipeline) override;
            void UnBindPipeline() override;

            void UpdateViewport(uint32_t width, uint32_t height, bool flipViewport) override {};
            static void MakeDefault();

        protected:
            static CommandBuffer* CreateFuncGL();

        private:
            bool primary;
            Pipeline* m_BoundPipeline = nullptr;
        };
    }
}
