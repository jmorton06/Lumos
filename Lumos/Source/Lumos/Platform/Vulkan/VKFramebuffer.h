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

            void Bind(uint32_t width, uint32_t height) const override {};
            void Bind() const override {};
            void UnBind() const override {};
            void Clear() override {};

            void AddTextureAttachment(RHIFormat format, Texture* texture) override {};
            void AddCubeTextureAttachment(RHIFormat format, CubeFace face, TextureCube* texture) override {};
            void AddShadowAttachment(Texture* texture) override {};
            void AddTextureLayer(int index, Texture* texture) override {};
            void GenerateFramebuffer() override {};

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
