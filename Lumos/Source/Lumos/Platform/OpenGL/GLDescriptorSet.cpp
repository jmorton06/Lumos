#include "Precompiled.h"
#include "GLDescriptorSet.h"
#include "GLShader.h"
#include "GLTexture.h"
#include "GLUniformBuffer.h"

namespace Lumos
{
    namespace Graphics
    {
        GLDescriptorSet::GLDescriptorSet(const DescriptorDesc& descriptorDesc)
        {
            m_Shader      = (GLShader*)descriptorDesc.shader;
            m_Descriptors = m_Shader->GetDescriptorInfo(descriptorDesc.layoutIndex).descriptors;

            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER)
                {
                    auto buffer = SharedPtr<Graphics::UniformBuffer>(Graphics::UniformBuffer::Create());
                    buffer->Init(descriptor.size, nullptr);
                    descriptor.buffer = buffer.get();

                    Buffer localStorage;
                    localStorage.Allocate(descriptor.size);
                    localStorage.InitialiseEmpty();

                    UniformBufferInfo info;
                    info.UB           = buffer;
                    info.LocalStorage = localStorage;
                    info.HasUpdated   = false;
                    info.m_Members    = descriptor.m_Members;
                    m_UniformBuffers.emplace(descriptor.name, info);

                    if(descriptor.name != "")
                    {
                        LUMOS_PROFILE_SCOPE("glUniformBlockBinding");
                        auto slot = descriptor.binding;
                        auto loc  = glGetUniformBlockIndex(m_Shader->GetHandleInternal(), descriptor.name.c_str());
                        GLCall(glUniformBlockBinding(m_Shader->GetHandleInternal(), loc, slot));
                    }
                }
            }
        }

        void GLDescriptorSet::Update()
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& bufferInfo : m_UniformBuffers)
            {
                if(bufferInfo.second.HasUpdated)
                {
                    bufferInfo.second.UB->SetData(bufferInfo.second.LocalStorage.Data);
                    bufferInfo.second.HasUpdated = false;
                }
            }
        }

        void GLDescriptorSet::SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.texture      = texture;
                    descriptor.textureType  = textureType;
                    descriptor.textureCount = 1;
                    return;
                }
            }
            LUMOS_LOG_WARN("Texture not found {0}", name);
        }

        void GLDescriptorSet::SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.textureCount = textureCount;
                    descriptor.textures     = texture;
                    descriptor.textureType  = textureType;
                    return;
                }
            }
            LUMOS_LOG_WARN("Texture not found {0}", name);
        }

        void GLDescriptorSet::SetBuffer(const std::string& name, UniformBuffer* buffer)
        {
            // TODO: Remove
            LUMOS_PROFILE_FUNCTION();
            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    descriptor.buffer = buffer;
                    return;
                }
            }

            LUMOS_LOG_WARN("Buffer not found {0}", name);
        }

        void GLDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data)
        {
            std::unordered_map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers.find(bufferName);
            if(itr != m_UniformBuffers.end())
            {
                for(auto& member : itr->second.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        itr->second.LocalStorage.Write(data, member.size, member.offset);
                        itr->second.HasUpdated = true;
                        return;
                    }
                }
            }

            LUMOS_LOG_WARN("Uniform not found {0}.{1}", bufferName, uniformName);
        }

        void GLDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size)
        {
            std::unordered_map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers.find(bufferName);
            if(itr != m_UniformBuffers.end())
            {
                for(auto& member : itr->second.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        itr->second.LocalStorage.Write(data, size, member.offset);
                        itr->second.HasUpdated = true;
                        return;
                    }
                }
            }

            LUMOS_LOG_WARN("Uniform not found {0}.{1}", bufferName, uniformName);
        }

        void GLDescriptorSet::SetUniformBufferData(const std::string& bufferName, void* data)
        {
            std::unordered_map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers.find(bufferName);
            if(itr != m_UniformBuffers.end())
            {
                itr->second.LocalStorage.Write(data, itr->second.LocalStorage.GetSize(), 0);
                itr->second.HasUpdated = true;
                return;
            }

            LUMOS_LOG_WARN("Uniform not found {0}.{1}", bufferName);
        }

        Graphics::UniformBuffer* GLDescriptorSet::GetUnifromBuffer(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    return descriptor.buffer;
                }
            }

            LUMOS_LOG_WARN("Buffer not found {0}", name);
            return nullptr;
        }

        void GLDescriptorSet::Bind(uint32_t offset)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Shader->Bind();

            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER)
                {
                    if(descriptor.textureCount == 1)
                    {
                        if(descriptor.texture)
                        {
                            descriptor.texture->Bind(descriptor.binding);
                            m_Shader->SetUniform1i(descriptor.name, descriptor.binding);
                        }
                    }
                    else
                    {
                        static const int MAX_TEXTURE_UNITS = 16;
                        int32_t samplers[MAX_TEXTURE_UNITS];

                        LUMOS_ASSERT(MAX_TEXTURE_UNITS >= descriptor.textureCount, "Texture Count greater than max");

                        for(uint32_t i = 0; i < descriptor.textureCount; i++)
                        {
                            if(descriptor.textures && descriptor.textures[i])
                            {
                                descriptor.textures[i]->Bind(descriptor.binding + i);
                                samplers[i] = i;
                            }
                        }
                        m_Shader->SetUniform1iv(descriptor.name, samplers, descriptor.textureCount);
                    }
                }
                else
                {
                    auto* buffer = dynamic_cast<GLUniformBuffer*>(descriptor.buffer);

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
                        // buffer->SetData(size, data);
                        auto bufferHandle = static_cast<GLUniformBuffer*>(buffer)->GetHandle();
                        auto slot         = descriptor.binding;
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferBase");
                            GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));
                        }

                        // if(buffer->GetDynamic())
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferRange");
                            GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));
                        }

                        //                        if(descriptor.name != "")
                        //                        {
                        //                            LUMOS_PROFILE_SCOPE("glUniformBlockBinding");
                        //                            auto loc = glGetUniformBlockIndex(m_Shader->GetHandleInternal(), descriptor.name.c_str());
                        //                            GLCall(glUniformBlockBinding(m_Shader->GetHandleInternal(), loc, slot));
                        //                        }
                    }
                }
            }
        }

        void GLDescriptorSet::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }

        DescriptorSet* GLDescriptorSet::CreateFuncGL(const DescriptorDesc& descriptorDesc)
        {
            return new GLDescriptorSet(descriptorDesc);
        }
    }
}
