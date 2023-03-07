#pragma once
#include "Core/Core.h"
#include "Graphics/RHI/Framebuffer.h"
#include "VK.h"
#include "VKTexture.h"
#include "VKRenderPass.h"

namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT VKFramebuffer : public Framebuffer
        {
        public:
            VKFramebuffer(const FramebufferDesc& frameBufferInfo);
            ~VKFramebuffer();

            const VkFramebuffer& GetFramebuffer() const { return m_Framebuffer; }

            void SetClearColour(const glm::vec4& colour) override {};

            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }

            static void MakeDefault();

        protected:
            static Framebuffer* CreateFuncVulkan(const FramebufferDesc&);

        private:
            uint32_t m_Width;
            uint32_t m_Height;
            uint32_t m_AttachmentCount;
            VkFramebuffer m_Framebuffer;
        };
    }
}
