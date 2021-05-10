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

        void GLDescriptorSet::Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            m_ImageInfos.clear();
            m_BufferInfos.clear();

            m_Shader->Bind();

            for(auto& imageInfo : imageInfos)
            {
                m_ImageInfos.push_back(imageInfo);

                if(imageInfo.count == 1)
                {
                    imageInfo.texture->Bind(imageInfo.binding);
                }
                else
                {
                    for(int i = 0; i < imageInfo.count; i++)
                    {
                        imageInfo.textures[i]->Bind(imageInfo.binding + i);
                    }
                }
                dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
            }

            for(auto& bufferInfo : bufferInfos)
            {
                m_BufferInfos.push_back(bufferInfo);
            }
        }

        void GLDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            m_ImageInfos.clear();

            m_Shader->Bind();

            for(auto& imageInfo : imageInfos)
            {
                m_ImageInfos.push_back(imageInfo);

                if(imageInfo.count == 1)
                {
                    imageInfo.texture->Bind(imageInfo.binding);
                }
                else
                {
                    for(int i = 0; i < imageInfo.count; i++)
                    {
                        imageInfo.textures[i]->Bind(imageInfo.binding + i);
                    }
                }
                dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
            }
        }

        void GLDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader->Bind();

            for(auto& bufferInfo : bufferInfos)
            {
                m_BufferInfos.push_back(bufferInfo);
            }
        }

        void GLDescriptorSet::Bind(uint32_t offset)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& imageInfo : m_ImageInfos)
            {
                if(imageInfo.count == 1)
                {
                    imageInfo.texture->Bind(imageInfo.binding);
                }
                else
                {
                    for(int i = 0; i < imageInfo.count; i++)
                    {
                        imageInfo.textures[i]->Bind(imageInfo.binding + i);
                    }
                }

                dynamic_cast<GLShader*>(m_Shader)->SetUniform1i(imageInfo.name, imageInfo.binding);
            }

            for(auto& bufferInfo : m_BufferInfos)
            {
                auto* buffer = dynamic_cast<GLUniformBuffer*>(bufferInfo.buffer);

                uint8_t* data;
                uint32_t size;

                if(buffer->GetDynamic())
                {
                    data = reinterpret_cast<uint8_t*>(buffer->GetBuffer()) + offset;
                    size = buffer->GetTypeSize();
                }
                else
                {
                    data = buffer->GetBuffer();
                    size = buffer->GetSize();
                }

                {
                    //buffer->SetData(size, data);
                    auto bufferHandle = static_cast<GLUniformBuffer*>(buffer)->GetHandle();
                    auto slot = bufferInfo.binding;
                    {
                        LUMOS_PROFILE_SCOPE("glBindBufferBase");
                        GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));
                    }

                    if(buffer->GetDynamic())
                    {
                        LUMOS_PROFILE_SCOPE("glBindBufferRange");
                        GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));
                    }

                    if(bufferInfo.name != "")
                    {
                        LUMOS_PROFILE_SCOPE("glUniformBlockBinding");
                        auto loc = glGetUniformBlockIndex(static_cast<GLShader*>(m_Shader)->GetHandleInternal(), bufferInfo.name.c_str());
                        GLCall(glUniformBlockBinding(static_cast<GLShader*>(m_Shader)->GetHandleInternal(), loc, slot));
                    }
                }
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
