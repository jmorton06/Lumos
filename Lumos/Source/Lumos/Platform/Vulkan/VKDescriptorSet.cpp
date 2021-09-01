#include "Precompiled.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKUtilities.h"
#include "VKUniformBuffer.h"
#include "VKTexture.h"
#include "VKDevice.h"
#include "VKRenderer.h"
#include "VKShader.h"

#define MAX_BUFFER_INFOS 32
#define MAX_IMAGE_INFOS 32
#define MAX_WRITE_DESCTIPTORS 32

namespace Lumos
{
    namespace Graphics
    {
        VKDescriptorSet::VKDescriptorSet(const DescriptorDesc& descriptorDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            m_FramesInFlight = uint32_t(VKRenderer::GetMainSwapChain()->GetSwapChainBufferCount());

            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool = VKRenderer::GetRenderer()->GetDescriptorPool();
            descriptorSetAllocateInfo.pSetLayouts = static_cast<Graphics::VKShader*>(descriptorDesc.shader)->GetDescriptorLayout(descriptorDesc.layoutIndex);
            descriptorSetAllocateInfo.descriptorSetCount = descriptorDesc.count;
            descriptorSetAllocateInfo.pNext = nullptr;

            m_BufferInfoPool = new VkDescriptorBufferInfo[MAX_BUFFER_INFOS];
            m_ImageInfoPool = new VkDescriptorImageInfo[MAX_IMAGE_INFOS];
            m_WriteDescriptorSetPool = new VkWriteDescriptorSet[MAX_WRITE_DESCTIPTORS];

            m_Shader = descriptorDesc.shader;

            for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
            {
                m_DescriptorSet[frame] = nullptr;
                VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDevice::GetHandle(), &descriptorSetAllocateInfo, &m_DescriptorSet[frame]));
                m_Descriptors[frame] = m_Shader->GetDescriptorInfo(descriptorDesc.layoutIndex);

                for(auto& descriptor : m_Descriptors[frame].descriptors)
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
                        info.UB = buffer;
                        info.LocalStorage = localStorage;
                        info.HasUpdated = false;
                        info.m_Members = descriptor.m_Members;
                        m_UniformBuffers[frame][descriptor.name] = info;
                    }
                }
            }
        }

        VKDescriptorSet::~VKDescriptorSet()
        {
            delete[] m_BufferInfoPool;
            delete[] m_ImageInfoPool;
            delete[] m_WriteDescriptorSetPool;
        }

        void VKDescriptorSet::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        DescriptorSet* VKDescriptorSet::CreateFuncVulkan(const DescriptorDesc& descriptorDesc)
        {
            return new VKDescriptorSet(descriptorDesc);
        }
    
        void TransitionImageToCorrectLayout(Texture* texture)
        {
            auto commandBuffer = Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            if(texture->GetType() == TextureType::COLOUR)
            {
                if(((VKTexture2D*)texture)->GetImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    ((VKTexture2D*)texture)->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VKCommandBuffer*)commandBuffer);
                }
            }
            else if(texture->GetType() == TextureType::DEPTH)
            {
                ((VKTextureDepth*)texture)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, (VKCommandBuffer*)commandBuffer);
            }
            else if(texture->GetType() == TextureType::DEPTHARRAY)
            {
                ((VKTextureDepthArray*)texture)->TransitionImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, (VKCommandBuffer*)commandBuffer);
            }
        }

        void VKDescriptorSet::Update()
        {
            LUMOS_PROFILE_FUNCTION();
            m_Dynamic = false;
            int descriptorWritesCount = 0;
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& bufferInfo : m_UniformBuffers[currentFrame])
            {
                // if(bufferInfo.second.HasUpdated)
                bufferInfo.second.UB->SetData(bufferInfo.second.LocalStorage.Data);
            }

            {
                int imageIndex = 0;
                int index = 0;

                for(auto& imageInfo : m_Descriptors[currentFrame].descriptors)
                {
                    if(imageInfo.type == DescriptorType::IMAGE_SAMPLER && (imageInfo.texture || imageInfo.textures))
                    {
                        if(imageInfo.textureCount == 1)
                        {
                            if(imageInfo.texture)
                            {
                                TransitionImageToCorrectLayout(imageInfo.texture);
                                
                                VkDescriptorImageInfo& des = *static_cast<VkDescriptorImageInfo*>(imageInfo.texture->GetDescriptorInfo());
                                m_ImageInfoPool[imageIndex].imageLayout = des.imageLayout;
                                m_ImageInfoPool[imageIndex].imageView = des.imageView;
                                m_ImageInfoPool[imageIndex].sampler = des.sampler;
                            }
                        }
                        else
                        {
                            if(imageInfo.textures)
                            {
                                for(uint32_t i = 0; i < imageInfo.textureCount; i++)
                                {
                                    TransitionImageToCorrectLayout(imageInfo.textures[i]);

                                    VkDescriptorImageInfo& des = *static_cast<VkDescriptorImageInfo*>(imageInfo.textures[i]->GetDescriptorInfo());
                                    m_ImageInfoPool[i + imageIndex].imageLayout = des.imageLayout;
                                    m_ImageInfoPool[i + imageIndex].imageView = des.imageView;
                                    m_ImageInfoPool[i + imageIndex].sampler = des.sampler;
                                }
                            }
                        }

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = m_DescriptorSet[currentFrame];
                        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writeDescriptorSet.dstBinding = imageInfo.binding;
                        writeDescriptorSet.pImageInfo = &m_ImageInfoPool[imageIndex];
                        writeDescriptorSet.descriptorCount = imageInfo.textureCount;

                        m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                        imageIndex++;
                        descriptorWritesCount++;
                    }
                    else
                    {
                        if(imageInfo.buffer)
                        {
                            m_BufferInfoPool[index].buffer = *dynamic_cast<VKUniformBuffer*>(imageInfo.buffer)->GetBuffer();
                            m_BufferInfoPool[index].offset = imageInfo.offset;
                            m_BufferInfoPool[index].range = imageInfo.size;

                            VkWriteDescriptorSet writeDescriptorSet {};
                            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                            writeDescriptorSet.dstSet = m_DescriptorSet[currentFrame];
                            writeDescriptorSet.descriptorType = VKUtilities::DescriptorTypeToVK(imageInfo.type);
                            writeDescriptorSet.dstBinding = imageInfo.binding;
                            writeDescriptorSet.pBufferInfo = &m_BufferInfoPool[index];
                            writeDescriptorSet.descriptorCount = 1;

                            m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                            index++;
                            descriptorWritesCount++;

                            if(imageInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                                m_Dynamic = true;
                        }
                    }
                }
            }
            vkUpdateDescriptorSets(VKDevice::Get().GetDevice(), descriptorWritesCount,
                m_WriteDescriptorSetPool, 0, nullptr);
        }

        void VKDescriptorSet::SetTexture(const std::string& name, Texture* texture, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();

            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.texture = texture;
                    descriptor.textureType = textureType;
                }
            }
        }

        void VKDescriptorSet::SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if(descriptor.type == DescriptorType::IMAGE_SAMPLER && descriptor.name == name)
                {
                    descriptor.textureCount = textureCount;
                    descriptor.textures = texture;
                    descriptor.textureType = textureType;
                }
            }
        }

        void VKDescriptorSet::SetBuffer(const std::string& name, UniformBuffer* buffer)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    descriptor.buffer = buffer;
                }
            }
        }

        Graphics::UniformBuffer* VKDescriptorSet::GetUnifromBuffer(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    return descriptor.buffer;
                }
            }

            LUMOS_LOG_WARN("Buffer not found {0}", name);
            return nullptr;
        }

        void VKDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            std::map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers[currentFrame].find(bufferName);
            if(itr != m_UniformBuffers[currentFrame].end())
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

        void VKDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            std::map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers[currentFrame].find(bufferName);
            if(itr != m_UniformBuffers[currentFrame].end())
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

        void VKDescriptorSet::SetUniformBufferData(const std::string& bufferName, void* data)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            std::map<std::string, UniformBufferInfo>::iterator itr = m_UniformBuffers[currentFrame].find(bufferName);
            if(itr != m_UniformBuffers[currentFrame].end())
            {
                itr->second.LocalStorage.Write(data, itr->second.LocalStorage.GetSize(), 0);
                itr->second.HasUpdated = true;
                return;
            }

            LUMOS_LOG_WARN("Uniform not found {0}.{1}", bufferName);
        }
    }
}
