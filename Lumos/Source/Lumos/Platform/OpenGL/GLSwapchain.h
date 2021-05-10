#pragma once
#include "Graphics/API/Swapchain.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLTexture2D;

        class GLSwapchain : public Swapchain
        {
        public:
            GLSwapchain(uint32_t width, uint32_t height);
            ~GLSwapchain();

            bool Init(bool vsync) override;

            Texture* GetCurrentImage() override;
            Texture* GetImage(uint32_t index) override { return nullptr; };
            uint32_t GetCurrentBufferIndex() const override;
            size_t GetSwapchainBufferCount() const override;

            static void MakeDefault();

        protected:
            static Swapchain* CreateFuncGL(uint32_t width, uint32_t height);

        private:
            std::vector<GLTexture2D*> swapChainBuffers;
            uint32_t currentBuffer = 0;

            uint32_t m_Width;
            uint32_t m_Height;
        };
    }
}
