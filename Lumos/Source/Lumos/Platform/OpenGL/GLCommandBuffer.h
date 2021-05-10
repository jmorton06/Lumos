#pragma once

#include "Graphics/API/CommandBuffer.h"

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
            void Execute(bool waitFence) override {};
            void ExecuteSecondary(CommandBuffer* primaryCmdBuffer) override;

            void UpdateViewport(uint32_t width, uint32_t height) override {};
            static void MakeDefault();

        protected:
            static CommandBuffer* CreateFuncGL();

        private:
            bool primary;
        };
    }
}
