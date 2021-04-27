#pragma once
#include "Graphics/API/Framebuffer.h"
#include "Platform/OpenGL/GL.h"
#include "GLTexture.h"

namespace Lumos
{
    namespace Graphics
    {
        enum class Format;

        class LUMOS_EXPORT GLFramebuffer : public Framebuffer
        {
        public:
            GLFramebuffer();
            GLFramebuffer(const FramebufferInfo& bufferInfo);
            ~GLFramebuffer();

            inline uint32_t GetFramebuffer() const { return m_Handle; }

            void GenerateFramebuffer() override;

            void Bind(uint32_t width, uint32_t height) const override;
            void Bind() const override;
            void UnBind() const override;
            void Clear() override { }
            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }

            GLenum GetAttachmentPoint(Graphics::TextureFormat format);

            inline void SetClearColour(const Maths::Vector4& colour) override { m_ClearColour = colour; }

            void AddTextureAttachment(TextureFormat format, Texture* texture) override;
            void AddCubeTextureAttachment(TextureFormat format, CubeFace face, TextureCube* texture) override;

            void AddShadowAttachment(Texture* texture) override;
            void AddTextureLayer(int index, Texture* texture) override;

            void Validate() override;

            static void MakeDefault();

        protected:
            static Framebuffer* CreateFuncGL(const FramebufferInfo& bufferInfo);

        private:
            uint32_t m_Handle;
            uint32_t m_Width, m_Height, m_ColourAttachmentCount;
            Maths::Vector4 m_ClearColour;
            std::vector<GLenum> m_AttachmentData;
            bool m_ScreenFramebuffer = false;
        };
    }
}
