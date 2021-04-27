#pragma once
#include "Core/Core.h"
#include "Graphics/API/Framebuffer.h"
#include "VK.h"
#include "VKTexture.h"
#include "VKRenderpass.h"

namespace Lumos
{
    namespace Graphics
    {
        class LUMOS_EXPORT VKFramebuffer : public Framebuffer
        {
        public:
            VKFramebuffer(const FramebufferInfo& frameBufferInfo);
            ~VKFramebuffer();

            const VkFramebuffer& GetFramebuffer() const { return m_Framebuffer; }

            void SetClearColour(const Maths::Vector4& colour) override {};

            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }

            void Bind(uint32_t width, uint32_t height) const override {};
            void Bind() const override {};
            void UnBind() const override {};
            void Clear() override {};

            void AddTextureAttachment(TextureFormat format, Texture* texture) override {};
            void AddCubeTextureAttachment(TextureFormat format, CubeFace face, TextureCube* texture) override {};
            void AddShadowAttachment(Texture* texture) override {};
            void AddTextureLayer(int index, Texture* texture) override {};
            void GenerateFramebuffer() override {};

            static void MakeDefault();

        protected:
            static Framebuffer* CreateFuncVulkan(const FramebufferInfo&);

        private:
            uint32_t m_Width;
            uint32_t m_Height;
            uint32_t m_AttachmentCount;
            VkFramebuffer m_Framebuffer;
        };
    }
}
