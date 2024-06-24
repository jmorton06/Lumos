#ifndef LUMOS_PLATFORM_MACOS
#include "Precompiled.h"
#endif
#include "GLSwapChain.h"
#include "GLCommandBuffer.h"
#include "Graphics/RHI/Framebuffer.h"

namespace Lumos
{
    namespace Graphics
    {

        GLSwapChain::GLSwapChain(uint32_t width, uint32_t height)
        {
            m_Width  = width;
            m_Height = height;
            //            FramebufferDesc info {};
            //            info.width = width;
            //            info.height = height;
            //            info.attachments = nullptr;
        }

        GLSwapChain::~GLSwapChain()
        {
            // for(auto& buffer : swapChainBuffers)
            //   delete buffer;
        }

        bool GLSwapChain::Init(bool vsync)
        {
            MainCommandBuffer = CreateSharedPtr<GLCommandBuffer>();
            return true;
        }

        Texture* GLSwapChain::GetCurrentImage()
        {
            return nullptr; // swapChainBuffers[0];
        }

        uint32_t GLSwapChain::GetCurrentBufferIndex() const
        {
            return 0;
        }

        size_t GLSwapChain::GetSwapChainBufferCount() const
        {
            return 1;
        }

        void GLSwapChain::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        SwapChain* GLSwapChain::CreateFuncGL(uint32_t width, uint32_t height)
        {
            return new GLSwapChain(width, height);
        }

        CommandBuffer* GLSwapChain::GetCurrentCommandBuffer()
        {
            return MainCommandBuffer.get();
        }
    }
}
