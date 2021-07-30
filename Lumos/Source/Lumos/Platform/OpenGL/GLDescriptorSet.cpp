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

        void GLDescriptorSet::Update()
        {
            LUMOS_PROFILE_FUNCTION();
        }
		
		void GLDescriptorSet::SetTexture(const std::string& name, Texture* texture, TextureType textureType) 
        {
			for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.texture = texture;
					descriptor.textureType = textureType;
                    descriptor.textureCount = 1;
                    return;
                }
            }
            LUMOS_LOG_WARN("Texture not found {0}", name);
        }
    
        void GLDescriptorSet::SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType)
        {
            for(auto& descriptor : m_Descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.textureCount = textureCount;
                    descriptor.textures = texture;
                    descriptor.textureType = textureType;
                    return;
                }
            }
            LUMOS_LOG_WARN("Texture not found {0}", name);

        }
		
        void GLDescriptorSet::SetBuffer(const std::string& name, UniformBuffer* buffer) 
        {
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

                        for(int i = 0; i < descriptor.textureCount; i++)
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
                        //buffer->SetData(size, data);
                        auto bufferHandle = static_cast<GLUniformBuffer*>(buffer)->GetHandle();
                        auto slot = descriptor.binding;
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferBase");
                            GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, bufferHandle));
                        }

                        if(buffer->GetDynamic())
                        {
                            LUMOS_PROFILE_SCOPE("glBindBufferRange");
                            GLCall(glBindBufferRange(GL_UNIFORM_BUFFER, slot, bufferHandle, offset, size));
                        }

                        if(descriptor.name != "")
                        {
                            LUMOS_PROFILE_SCOPE("glUniformBlockBinding");
                            auto loc = glGetUniformBlockIndex(m_Shader->GetHandleInternal(), descriptor.name.c_str());
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
