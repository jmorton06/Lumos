#include "Precompiled.h"
#include "GLFramebuffer.h"

#include "Platform/OpenGL/GLDebug.h"
#include "Graphics/API/Texture.h"

namespace Lumos
{
    namespace Graphics
    {
        GLFramebuffer::GLFramebuffer()
            : m_Width(0)
            , m_Height(0)
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glGenFramebuffers(1, &m_Handle));
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_Handle));
            m_ColourAttachmentCount = 0;
        }

        GLFramebuffer::GLFramebuffer(const FramebufferInfo& bufferInfo)
        {
            LUMOS_PROFILE_FUNCTION();
            m_ScreenFramebuffer = bufferInfo.screenFBO;
            m_Width = bufferInfo.width;
            m_Height = bufferInfo.height;
            m_ColourAttachmentCount = 0;

            if(m_ScreenFramebuffer)
            {
                GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
            }
            else
            {
                GLCall(glGenFramebuffers(1, &m_Handle));
                GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_Handle));

                for(uint32_t i = 0; i < bufferInfo.attachmentCount; i++)
                {
                    switch(bufferInfo.attachmentTypes[i])
                    {
                    case TextureType::COLOUR:
                        AddTextureAttachment(TextureFormat::RGBA8, bufferInfo.attachments[i]);
                        break;
                    case TextureType::DEPTH:
                        AddTextureAttachment(TextureFormat::DEPTH, bufferInfo.attachments[i]);
                        break;
                    case TextureType::DEPTHARRAY:
                        AddTextureLayer(bufferInfo.layer, bufferInfo.attachments[i]);
                        break;
                    case TextureType::OTHER:
                        UNIMPLEMENTED;
                        break;
                    case TextureType::CUBE:
                        UNIMPLEMENTED;
                        break;
                    }
                }

                GLCall(glDrawBuffers(static_cast<GLsizei>(m_AttachmentData.size()), m_AttachmentData.data()));

                Validate();
            }
        }

        GLFramebuffer::~GLFramebuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_ScreenFramebuffer)
                GLCall(glDeleteFramebuffers(1, &m_Handle));
        }

        void GLFramebuffer::GenerateFramebuffer()
        {
            LUMOS_PROFILE_FUNCTION();
            if(!m_ScreenFramebuffer)
                GLCall(glGenFramebuffers(1, &m_Handle));
        }

        void GLFramebuffer::Bind(uint32_t width, uint32_t height) const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ScreenFramebuffer ? 0 : m_Handle));
            GLCall(glViewport(0, 0, width, height));

            if(!m_ScreenFramebuffer)
                GLCall(glDrawBuffers(static_cast<GLsizei>(m_AttachmentData.size()), m_AttachmentData.data()));
        }

        void GLFramebuffer::UnBind() const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
        }

        GLenum GLFramebuffer::GetAttachmentPoint(Graphics::TextureFormat format)
        {
            LUMOS_PROFILE_FUNCTION();
            if(Graphics::Texture::IsDepthStencilFormat(format))
            {
                return GL_DEPTH_STENCIL_ATTACHMENT;
            }

            if(Graphics::Texture::IsStencilFormat(format))
            {
                return GL_STENCIL_ATTACHMENT;
            }

            if(Graphics::Texture::IsDepthFormat(format))
            {
                return GL_DEPTH_ATTACHMENT;
            }

            GLenum value = GL_COLOR_ATTACHMENT0 + m_ColourAttachmentCount;
            m_ColourAttachmentCount++;
            return value;
        }

        void GLFramebuffer::Bind() const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_ScreenFramebuffer ? 0 : m_Handle));
        }

        void GLFramebuffer::AddTextureAttachment(const Graphics::TextureFormat format, Texture* texture)
        {
            LUMOS_PROFILE_FUNCTION();
            GLenum attachment = GetAttachmentPoint(format);

            if(attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT)
            {
                m_AttachmentData.emplace_back(attachment);
            }
            GLCall(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, (GLuint)(size_t)texture->GetHandle(), 0));
        }

        void GLFramebuffer::AddCubeTextureAttachment(const Graphics::TextureFormat format, const CubeFace face, TextureCube* texture)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t faceID = 0;

            GLenum attachment = GetAttachmentPoint(format);
            if(attachment != GL_DEPTH_ATTACHMENT && attachment != GL_STENCIL_ATTACHMENT && attachment != GL_DEPTH_STENCIL_ATTACHMENT)
            {
                m_AttachmentData.emplace_back(attachment);
            }

            switch(face)
            {
            case CubeFace::PositiveX:
                faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                break;
            case CubeFace::NegativeX:
                faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
                break;
            case CubeFace::PositiveY:
                faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
                break;
            case CubeFace::NegativeY:
                faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
                break;
            case CubeFace::PositiveZ:
                faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
                break;
            case CubeFace::NegativeZ:
                faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
                break;
            }

            GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, faceID, (GLuint)(size_t)texture->GetHandle(), 0));
        }

        void GLFramebuffer::AddShadowAttachment(Texture* texture)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef LUMOS_PLATFORM_MOBILE
            GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)texture->GetHandle(), 0, 0));
#else
            GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, 0));
#endif
            GLCall(glDrawBuffers(0, GL_NONE));
        }

        void GLFramebuffer::AddTextureLayer(int index, Texture* texture)
        {
            LUMOS_PROFILE_FUNCTION();
#ifdef LUMOS_PLATFORM_MOBILE
            GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, index));
#else
            GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, index));
#endif
        }

        void GLFramebuffer::Validate()
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if(status != GL_FRAMEBUFFER_COMPLETE)
            {
                LUMOS_LOG_CRITICAL("Unable to create Framebuffer! StatusCode: {0}", status);
            }
        }

        void GLFramebuffer::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        Framebuffer* GLFramebuffer::CreateFuncGL(const FramebufferInfo& bufferInfo)
        {
            return new GLFramebuffer(bufferInfo);
        }
    }
}
