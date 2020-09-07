#include "Precompiled.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLShader.h"
#include "GLUniformBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        GLDescriptorSet::GLDescriptorSet(const DescriptorInfo& info)
        {
            m_Shader = info.shader;
        }

        void GLDescriptorSet::Update(std::vector<ImageInfo> &imageInfos, std::vector<BufferInfo> &bufferInfos)
        {
			m_ImageInfos.clear();
			m_BufferInfos.clear();

            m_Shader->Bind();

            for(auto& imageInfo : imageInfos)
            {
				m_ImageInfos.push_back(imageInfo);
				for (int i = 0; i < imageInfo.count; i++)
				{
					imageInfo.texture[i]->Bind(imageInfo.binding + i);
				}
				dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
            }

			for (auto& bufferInfo : bufferInfos)
			{
				m_BufferInfos.push_back(bufferInfo);
			}
        }

        void GLDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
        {
			m_ImageInfos.clear();

            m_Shader->Bind();

            for(auto& imageInfo : imageInfos)
            {
				m_ImageInfos.push_back(imageInfo);
				for (int i = 0; i < imageInfo.count; i++)
				{
					imageInfo.texture[i]->Bind(imageInfo.binding + i);
				}
				dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
            }

        }

        void GLDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
        {
            m_Shader->Bind();

			for (auto& bufferInfo : bufferInfos)
			{
				m_BufferInfos.push_back(bufferInfo);
			}
        }

		void GLDescriptorSet::Bind(u32 offset)
		{
			for (auto& imageInfo : m_ImageInfos)
			{
				for(int i = 0; i < imageInfo.count; i++)
				{
                    if(imageInfo.texture[i])
                        imageInfo.texture[i]->Bind(imageInfo.binding + i);
				}

				dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
			}

            for (auto& bufferInfo : m_BufferInfos)
			{
				auto* buffer = dynamic_cast<GLUniformBuffer*>(bufferInfo.buffer);

                u8* data;
                u32 size;

                if(buffer->GetDynamic())
                {
                    data = reinterpret_cast<u8*>(buffer->GetBuffer()) + offset;
                    size = buffer->GetTypeSize();
                }
                else
                {
                    data = buffer->GetBuffer();
                    size  = buffer->GetSize();
                }

                {
                    //buffer->SetData(size, data);
                    auto bufferHandle = static_cast<GLUniformBuffer*>(buffer)->GetHandle();
                    auto slot = bufferInfo.binding;
                    GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));

					if(buffer->GetDynamic())
						GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));

					if (bufferInfo.name != "")
					{
						auto loc = glGetUniformBlockIndex(static_cast<GLShader*>(m_Shader)->GetHandleInternal(), bufferInfo.name.c_str());
						GLCall(glUniformBlockBinding(static_cast<GLShader*>(m_Shader)->GetHandleInternal(), loc, slot));
					}
                }
			}

			for (auto pc : m_PushConstants)
				static_cast<GLShader*>(m_Shader)->SetUserUniformBuffer(pc.shaderStage, pc.data, pc.size);
		}

        void GLDescriptorSet::SetPushConstants(std::vector<PushConstant>& pushConstants)
		{
			m_PushConstants.clear();
			for (auto& pushConstant : pushConstants)
			{
				m_PushConstants.push_back(pushConstant);
			}
		}

		void GLDescriptorSet::MakeDefault()
		{
			CreateFunc = CreateFuncGL;
		}

		DescriptorSet* GLDescriptorSet::CreateFuncGL(const DescriptorInfo& info)
		{
			return new GLDescriptorSet(info);
		}
    }
}
