#include "Precompiled.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLUniformBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        GLDescriptorSet::GLDescriptorSet(const DescriptorDesc& info)
        {
            m_Shader = (GLShader*)info.shader;
            m_Descriptors = m_Shader->GetDescriptorInfo(info.layoutIndex).descriptors;
        }

        void GLDescriptorSet::Update(std::vector<Descriptor>& descriptors)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Descriptors = descriptors;
        }

        void GLDescriptorSet::Bind(uint32_t offset)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader->Bind();

            for(auto& imageInfo : m_Descriptors)
            {
                if(imageInfo.type == DescriptorType::IMAGE_SAMPLER)
                {
                    if(imageInfo.textureCount == 1)
                    {
                        imageInfo.texture->Bind(imageInfo.binding);
                        m_Shader->SetUniform1i(imageInfo.name, imageInfo.binding);
                    }
                    else
                    {
                        static const int MAX_TEXTURE_UNITS = 16;
                        int32_t samplers[MAX_TEXTURE_UNITS];

                        LUMOS_ASSERT(MAX_TEXTURE_UNITS >= imageInfo.textureCount, "Texture Count greater than max");

                        for(int i = 0; i < imageInfo.textureCount; i++)
                        {
                            imageInfo.textures[i]->Bind(imageInfo.binding + i);
                            samplers[i] = i;
                        }
                        m_Shader->SetUniform1iv(imageInfo.name, samplers, imageInfo.textureCount);
                    }
                }
                else
                {
                    auto* buffer = dynamic_cast<GLUniformBuffer*>(imageInfo.buffer);

                    if(!buffer)
                        break;

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
                        auto slot = imageInfo.binding;
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferBase");
                            GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));
                        }

                        if(buffer->GetDynamic())
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferRange");
                            GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));
                        }

                        if(imageInfo.name != "")
                        {
                            LUMOS_PROFILE_SCOPE("glUniformBlockBinding");
                            auto loc = glGetUniformBlockIndex(m_Shader->GetHandleInternal(), imageInfo.name.c_str());
                            GLCall(glUniformBlockBinding(m_Shader->GetHandleInternal(), loc, slot));
                        }
                    }
                }
            }
        }

        void GLDescriptorSet::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        DescriptorSet* GLDescriptorSet::CreateFuncGL(const DescriptorDesc& info)
        {
            return new GLDescriptorSet(info);
        }
    }
}
