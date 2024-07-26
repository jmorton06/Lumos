#pragma once
#include "Graphics/RHI/Framebuffer.h"
#include "Platform/OpenGL/GL.h"
#include "GLTexture.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class RHIFormat : uint8_t;

        class LUMOS_EXPORT GLFramebuffer : public Framebuffer
        {
        public:
            GLFramebuffer();
            GLFramebuffer(const FramebufferDesc& frameBufferDesc);
            ~GLFramebuffer();

            inline uint32_t GetFramebuffer() const { return m_Handle; }

            void GenerateFramebuffer();

            void Bind(uint32_t width, uint32_t height) const;
            void Bind() const;
            void UnBind() const;
            void Clear() { }
            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }

            GLenum GetAttachmentPoint(Graphics::RHIFormat format);

            inline void SetClearColour(const Vec4& colour) override { m_ClearColour = colour; }

            void AddTextureAttachment(RHIFormat format, Texture* texture, uint32_t mipLevel = 0);
            void AddCubeTextureAttachment(RHIFormat format, CubeFace face, TextureCube* texture, uint32_t mipLevel = 0);

            void AddShadowAttachment(Texture* texture);
            void AddTextureLayer(int index, Texture* texture);

            void Validate() override;

            static void MakeDefault();

        protected:
            static Framebuffer* CreateFuncGL(const FramebufferDesc& frameBufferDesc);

        private:
            uint32_t m_Handle;
            uint32_t m_Width, m_Height, m_ColourAttachmentCount;
            Vec4 m_ClearColour;
            std::vector<GLenum> m_AttachmentData;
            bool m_ScreenFramebuffer = false;
        };
    }
}
