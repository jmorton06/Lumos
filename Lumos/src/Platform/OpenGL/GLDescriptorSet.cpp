#include "LM.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"
#include "Textures/GLTexture2D.h"
#include "GLShader.h"
#include "GLUniformBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        GLDescriptorSet::GLDescriptorSet(DescriptorInfo& info)
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

		void GLDescriptorSet::Bind(uint offset)
		{
			for (auto& imageInfo : m_ImageInfos)
			{
				for(int i = 0; i < imageInfo.count; i++)
				{
					imageInfo.texture[i]->Bind(imageInfo.binding + i);
				}

				dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
			}

            for (auto& bufferInfo : m_BufferInfos)
			{
				auto* buffer = dynamic_cast<GLUniformBuffer*>(bufferInfo.buffer);

                byte* data;
                uint size;

                if(buffer->GetDynamic())
                {
                    data = reinterpret_cast<byte*>(buffer->GetBuffer()) + offset;
                    size = buffer->GetTypeSize();
                }
                else
                {
                    data = buffer->GetBuffer();
                    size  = buffer->GetSize();
                }

                if(!bufferInfo.name.empty())
                {
                    static_cast<GLUniformBuffer*>(buffer)->Bind(0, dynamic_cast<GLShader*>(m_Shader), bufferInfo.name);
                }
                else
                {
                    if(bufferInfo.systemUniforms)
                        m_Shader->SetSystemUniformBuffer(bufferInfo.shaderType, data, size);
                    else
                        m_Shader->SetUserUniformBuffer(bufferInfo.shaderType, data, size);
                }

			}

            for(auto pc : m_PushConstants)
                //((GLShader*)m_Shader)->SetUniform("PushConstant", (byte*)pc.data);
	            dynamic_cast<GLShader*>(m_Shader)->SetUniform1i("PushConstant", static_cast<int32>(pc.data[0])); //TEMP
		}

        void GLDescriptorSet::SetPushConstants(std::vector<PushConstant>& pushConstants)
		{
			m_PushConstants.clear();
			for (auto& pushConstant : pushConstants)
			{
				m_PushConstants.push_back(pushConstant);
			}
		}
    }
}
