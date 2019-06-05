#include "LM.h"
#include "GLTextureDepthArray.h"

#include "../GL.h"


namespace Lumos
{
	namespace Graphics
	{
		GLTextureDepthArray::GLTextureDepthArray(uint width, uint height, uint count)
			: m_Width(width), m_Height(height), m_Count(count)
		{
			GLTextureDepthArray::Init();
		}

		GLTextureDepthArray::~GLTextureDepthArray()
		{
			GLCall(glDeleteTextures(1, &m_Handle));
		}

		void GLTextureDepthArray::Bind(uint slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, m_Handle));
		}

		void GLTextureDepthArray::Unbind(uint slot) const
		{
			GLCall(glActiveTexture(GL_TEXTURE0 + slot));
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
		}

		void GLTextureDepthArray::Init()
		{
			GLCall(glGenTextures(1, &m_Handle));
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, m_Handle));

			GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, m_Count, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
#ifndef LUMOS_PLATFORM_MOBILE
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
			// GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY));
#endif
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));

		}

		void GLTextureDepthArray::Resize(uint width, uint height, uint count)
		{
			m_Width = width;
			m_Height = height;
			m_Count = count;

			GLCall(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT, m_Width, m_Height, m_Count, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr));

			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
#ifndef LUMOS_PLATFORM_MOBILE
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
			//GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY));
#endif
			GLCall(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));
			GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY, 0));
		}
	}
}
