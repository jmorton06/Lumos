#include "LM.h"
#include "GLTexture2D.h"
#include "Platform/OpenGL/GL.h"
#include "Platform/OpenGL/GLTools.h"
#include "Platform/OpenGL/GLShader.h"
#include "Utilities/LoadImage.h"

namespace Lumos
{
	namespace Graphics
	{
		GLTexture2D::GLTexture2D() : m_Width(0), m_Height(0)
		{
			glGenTextures(1, &m_Handle);
		}

		GLTexture2D::GLTexture2D(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
			: m_FileName(""), m_Name(""), m_Parameters(parameters), m_LoadOptions(loadOptions)
		{
			m_Name = "";
			m_Parameters = parameters;
			m_LoadOptions = loadOptions;
			m_Handle = Load(data);
		}

		GLTexture2D::GLTexture2D(const String& name, const String& filename, const TextureParameters parameters, const TextureLoadOptions loadOptions)
			: m_FileName(filename), m_Name(name), m_Parameters(parameters), m_LoadOptions(loadOptions)
		{
			m_Handle = Load(nullptr);
		}

		GLTexture2D::~GLTexture2D()
		{
			GLCall(glDeleteTextures(1, &m_Handle));
		}

		u32 GLTexture2D::LoadTexture(void* data) const
		{
			u32 handle;
			GLCall(glGenTextures(1, &handle));
			GLCall(glBindTexture(GL_TEXTURE_2D, handle));
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_Parameters.filter == TextureFilter::LINEAR ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST));
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_Parameters.filter == TextureFilter::LINEAR ? GL_LINEAR : GL_NEAREST));
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLTools::TextureWrapToGL(m_Parameters.wrap)));
			GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLTools::TextureWrapToGL(m_Parameters.wrap)));


			u32 format = GLTools::TextureFormatToGL(m_Parameters.format);
			GLCall(glTexImage2D(GL_TEXTURE_2D, 0, format, m_Width, m_Height, 0, GLTools::TextureFormatToInternalFormat(format), GL_UNSIGNED_BYTE, data ? data : NULL));
			GLCall(glGenerateMipmap(GL_TEXTURE_2D));
#ifdef LUMOS_DEBUG
			GLCall(glBindTexture(GL_TEXTURE_2D, 0));
#endif

			return handle;
		}

		u32 GLTexture2D::Load(void* data)
		{
			byte* pixels = nullptr;

			if (data != nullptr)
			{
				pixels = reinterpret_cast<byte*>(data);
			}
			else
			{
				if (m_FileName != "")
				{
					pixels = LoadTextureData();
				}
			}
			
			u32 handle = LoadTexture(pixels);

			if (pixels != nullptr)
				delete[] pixels;

			return handle;
		}

		void GLTexture2D::SetData(const void* pixels)
		{
			GLCall(glBindTexture(GL_TEXTURE_2D, m_Handle));
			GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GLTools::TextureFormatToGL(m_Parameters.format), GL_UNSIGNED_BYTE, pixels));
			GLCall(glGenerateMipmap(GL_TEXTURE_2D));
		}

		void GLTexture2D::Bind(u32 slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_2D, m_Handle));
		}

		void GLTexture2D::Unbind(u32 slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_2D, 0));
		}

		void GLTexture2D::BuildTexture(const TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow)
		{
			m_Width = width;
			m_Height = height;
			m_Name = "Texture Attachment";

			u32 Format = GLTools::TextureFormatToGL(internalformat);
			u32 Format2 = GLTools::TextureFormatToInternalFormat(Format);

			glBindTexture(GL_TEXTURE_2D, m_Handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			if (samplerShadow)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
#ifndef LUMOS_PLATFORM_MOBILE
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
#endif
			}

			glTexImage2D(GL_TEXTURE_2D, 0, Format, width, height, 0, Format2, depth ? GL_UNSIGNED_BYTE : GL_FLOAT, nullptr);
		}

		byte* GLTexture2D::LoadTextureData()
		{
			byte* pixels = nullptr;
			if (m_FileName != "NULL")
			{
				u32 bits;
				pixels = Lumos::LoadImageFromFile(m_FileName.c_str(), &m_Width, &m_Height, &bits, !m_LoadOptions.flipY);
				m_Parameters.format = GLTools::BitsToTextureFormat(bits);
			}
			return pixels;
		};
	}
}
