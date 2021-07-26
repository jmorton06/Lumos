
#include "Graphics/RHI/CommandBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        class NoneCommandBuffer : public CommandBuffer
        {
        public:
            NoneCommandBuffer();
            ~NoneCommandBuffer();

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
        };
    }
}