#include "LM.h"
#include "GLFramebuffer.h"

#include "Platform/OpenGL/GLDebug.h"
#include "Graphics/API/Textures/TextureCube.h"

namespace Lumos
{
	Attachment GetColourAttachment(int index)
	{
		switch(index)
		{
			case 0 : return Attachment::Colour0; break;
			case 1 : return Attachment::Colour1; break;
			case 2 : return Attachment::Colour2; break;
			case 3 : return Attachment::Colour3; break;
			case 4 : return Attachment::Colour4; break;
			case 5 : return Attachment::Colour5; break;
			default : return Attachment::Colour0; break;
		}
	}

	GLFramebuffer::GLFramebuffer() : m_Width(0), m_Height(0)
	{
		GLCall(glGenFramebuffers(1, &m_Handle));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_Handle));
	}

    GLFramebuffer::GLFramebuffer(FramebufferInfo bufferInfo)
    {
        GLCall(glGenFramebuffers(1, &m_Handle));
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_Handle));

		m_Width  = bufferInfo.width;
		m_Height = bufferInfo.height;

		m_AttachmentCount = bufferInfo.attachmentCount;
		m_ColourAttachmentCount = 0;
		for(uint i = 0; i < m_AttachmentCount; i++)
		{
			switch (bufferInfo.attachmentTypes[i])
			{
			case TextureType::COLOUR 	 : AddTextureAttachment(GetColourAttachment(m_ColourAttachmentCount),bufferInfo.attachments[i]); m_ColourAttachmentCount++;  break;
			case TextureType::DEPTH  	 : AddTextureAttachment(Attachment::Depth,bufferInfo.attachments[i]); break;
			case TextureType::DEPTHARRAY : AddTextureLayer(bufferInfo.layer, bufferInfo.attachments[i]); break;
			case TextureType::OTHER  	 : AddTextureAttachment(GetColourAttachment(m_ColourAttachmentCount),bufferInfo.attachments[i]); m_ColourAttachmentCount++;  break;
            case TextureType::CUBE   : UNIMPLEMENTED; break;
			}
		}

		if(GLRenderer::s_Instance != nullptr)
			GLRenderer::s_Instance->SetRenderTargets(m_ColourAttachmentCount);
    }

	GLFramebuffer::~GLFramebuffer()
	{
		GLCall(glDeleteFramebuffers(1, &m_Handle));
	}

	void GLFramebuffer::GenerateFramebuffer()
	{
		GLCall(glGenFramebuffers(1, &m_Handle));
	}

	void GLFramebuffer::UnBind() const
	{
#ifdef LUMOS_DEBUG
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
	}

	void GLFramebuffer::Bind(uint width, uint height) const
	{
		GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Handle));
		GLCall(glViewport(0, 0, width, height));

		if(GLRenderer::s_Instance != nullptr)
			GLRenderer::s_Instance->SetRenderTargets(m_ColourAttachmentCount);
	}

	void GLFramebuffer::Bind() const
	{
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_Handle));
	}

	void GLFramebuffer::AddTextureAttachment(const Attachment attachmentType, Texture* texture)
	{
		uint attachment;
		switch (attachmentType)
		{
		case Attachment::Colour0: attachment = GL_COLOR_ATTACHMENT0; break;
		case Attachment::Colour1: attachment = GL_COLOR_ATTACHMENT1; break;
		case Attachment::Colour2: attachment = GL_COLOR_ATTACHMENT2; break;
		case Attachment::Colour3: attachment = GL_COLOR_ATTACHMENT3; break;
		case Attachment::Colour4: attachment = GL_COLOR_ATTACHMENT4; break;
		case Attachment::Colour5: attachment = GL_COLOR_ATTACHMENT5; break;
		case Attachment::Depth: attachment = GL_DEPTH_ATTACHMENT; break;
		case Attachment::DepthArray: attachment = GL_DEPTH_ATTACHMENT; break;
		case Attachment::Stencil: attachment = GL_STENCIL_ATTACHMENT; break;

		default: attachment = 0;
		}
		GLCall(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, attachment, GL_TEXTURE_2D, (GLuint)(size_t)texture->GetHandle(), 0));
	}

	void GLFramebuffer::AddCubeTextureAttachment(const Attachment attachmentType, const CubeFace face, TextureCube* texture)
	{
		uint attachment;
		uint faceID = 0;

		switch (attachmentType)
		{
		case Attachment::Colour0: attachment = GL_COLOR_ATTACHMENT0; break;
		case Attachment::Colour1: attachment = GL_COLOR_ATTACHMENT1; break;
		case Attachment::Colour2: attachment = GL_COLOR_ATTACHMENT2; break;
		case Attachment::Colour3: attachment = GL_COLOR_ATTACHMENT3; break;
		case Attachment::Colour4: attachment = GL_COLOR_ATTACHMENT4; break;
		case Attachment::Depth:   attachment = GL_DEPTH_ATTACHMENT; break;
		case Attachment::Stencil: attachment = GL_STENCIL_ATTACHMENT; break;

		default: attachment = 0;
		}

		switch (face)
		{
		case CubeFace::PositiveX: faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_X; break;
		case CubeFace::NegativeX: faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; break;
		case CubeFace::PositiveY: faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Y; break;
		case CubeFace::NegativeY: faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y; break;
		case CubeFace::PositiveZ: faceID = GL_TEXTURE_CUBE_MAP_POSITIVE_Z; break;
		case CubeFace::NegativeZ: faceID = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; break;
		}

		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, faceID, (GLuint)(size_t)texture->GetHandle(), 0));
	}

	void GLFramebuffer::AddShadowAttachment(Texture* texture)
	{
#ifdef LUMOS_PLATFORM_MOBILE
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)texture->GetHandle(), 0, 0));
#else
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, 0));
#endif
		GLCall(glDrawBuffers(0, GL_NONE));
	}

	void GLFramebuffer::AddTextureLayer(int index, Texture* texture)
	{
#ifdef LUMOS_PLATFORM_MOBILE
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, index));
#else
		GLCall(glFramebufferTextureLayer(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT, (GLuint)(size_t)texture->GetHandle(), 0, index));
#endif

	}

	void GLFramebuffer::Validate()
	{
		uint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			LUMOS_CORE_ERROR("Unable to create Screen Framebuffer! StatusCode: {0}", status);
		}
	}
}
