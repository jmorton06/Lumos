#pragma once
#include "Graphics/RHI/SwapChain.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLCommandBuffer;
        class GLTexture2D;

        class GLSwapChain : public SwapChain
        {
        public:
            GLSwapChain(uint32_t width, uint32_t height);
            ~GLSwapChain();
            bool Init(bool vsync, Window* window) override { return Init(vsync); };
            bool Init(bool vsync) override;

            Texture* GetCurrentImage() override;
            Texture* GetImage(uint32_t index) override { return nullptr; };
            uint32_t GetCurrentBufferIndex() const override;
            uint32_t GetCurrentImageIndex() const override { return 0; };
            CommandBuffer* GetCurrentCommandBuffer() override;
            void OnResize(uint32_t width, uint32_t height)
            {
                m_Width  = width;
                m_Height = height;
            }
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }

            size_t GetSwapChainBufferCount() const override;

            static void MakeDefault();

        protected:
            static SwapChain* CreateFuncGL(uint32_t width, uint32_t height);

        private:
            std::vector<GLTexture2D*> swapChainBuffers;
            SharedPtr<GLCommandBuffer> MainCommandBuffer;
            uint32_t currentBuffer = 0;

            uint32_t m_Width;
            uint32_t m_Height;
        };
    }
}
