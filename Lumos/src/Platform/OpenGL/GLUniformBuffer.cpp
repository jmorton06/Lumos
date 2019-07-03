#include "LM.h"
#include "GLUniformBuffer.h"
#include "GL.h"
#include "GLDebug.h"
#include "GLShader.h"

namespace Lumos
{
    namespace Graphics
    {
		GLUniformBuffer::GLUniformBuffer()
		{	
			glGenBuffers(1, &m_Handle);
		}

		GLUniformBuffer::~GLUniformBuffer()
		{
			GLCall(glDeleteBuffers(1, &m_Handle));
		}
		
		void GLUniformBuffer::Init(uint32_t size, const void * data)
		{ 
			m_Data = (byte*)data; 
			m_Size = size;
			glBindBuffer(GL_UNIFORM_BUFFER, m_Handle);
			glBufferData(GL_UNIFORM_BUFFER, m_Size, m_Data, GL_DYNAMIC_DRAW);
		}

		void GLUniformBuffer::SetData(uint32_t size, const void * data) 
		{
			m_Data = (byte*)data; 
			m_Size = size;

			glBindBuffer(GL_UNIFORM_BUFFER, m_Handle);
			GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
			memcpy(p, m_Data, m_Size);
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		void GLUniformBuffer::SetDynamicData(uint32_t size, uint32_t typeSize, const void * data) 
		{ 
			m_Data = (byte*)data; 
			m_Size = size; 
			m_Dynamic = true; 
			m_DynamicTypeSize = typeSize;

			glBindBuffer(GL_UNIFORM_BUFFER, m_Handle);
			GLvoid* p = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
			memcpy(p, m_Data, m_Size);
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		void GLUniformBuffer::Bind(u32 slot, GLShader* shader, String& name)
		{
			GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, m_Handle));
			u32 location = glGetUniformBlockIndex(shader->GetHandle(), name.c_str());
			GLCall(glUniformBlockBinding(shader->GetHandle(), location, slot));
		}
	}
}
