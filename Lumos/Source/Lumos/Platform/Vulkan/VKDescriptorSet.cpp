#include "Precompiled.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKUtilities.h"
#include "VKUniformBuffer.h"
#include "VKTexture.h"
#include "VKDevice.h"
#include "VKRenderer.h"
#include "VKShader.h"

namespace Lumos
{
    namespace Graphics
    {
        static uint32_t g_DescriptorSetCount = 0;
        VKDescriptorSet::VKDescriptorSet(const DescriptorDesc& descriptorDesc)
        {
            LUMOS_PROFILE_FUNCTION();
            m_FramesInFlight = uint32_t(VKRenderer::GetMainSwapChain()->GetSwapChainBufferCount());

            m_Shader                            = descriptorDesc.shader;
            DescriptorSetInfo descriptorSetInfo = m_Shader->GetDescriptorInfo(descriptorDesc.layoutIndex);

            for(auto& descriptor : descriptorSetInfo.descriptors)
            {
                DescriptorData info;
                info.HasUpdated[0] = false;
                info.HasUpdated[1] = false;
                info.HasUpdated[2] = false;
                info.Desc          = descriptor;
                info.Binding       = descriptor.binding;

                if(descriptor.type == DescriptorType::UNIFORM_BUFFER)
                {
                    for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
                    {
                        // Uniform Buffer per frame in flight
                        auto buffer = SharedPtr<Graphics::UniformBuffer>(Graphics::UniformBuffer::Create());
                        buffer->Init(descriptor.size, nullptr);
                        m_UniformBuffers[frame][descriptor.binding] = buffer;
                    }

                    Buffer localStorage;
                    localStorage.Allocate(descriptor.size);
                    localStorage.InitialiseEmpty();
                    info.LocalStorage = localStorage;
                }
                info.valid                           = true;
                m_DescriptorData[descriptor.binding] = info;
            }

            for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
            {
                m_DescriptorDirty[frame]   = true;
                m_DescriptorUpdated[frame] = false;
                m_DescriptorSet[frame]     = nullptr;
                g_DescriptorSetCount++;
                auto layout = static_cast<Graphics::VKShader*>(descriptorDesc.shader)->GetDescriptorLayout(descriptorDesc.layoutIndex);
                VKRenderer::GetRenderer()->AllocateDescriptorSet(&m_DescriptorSet[frame], m_DescriptorPoolCreatedFrom[frame], *layout, descriptorDesc.count);
            }
        }

        VKDescriptorSet::~VKDescriptorSet()
        {
            for(uint32_t frame = 0; frame < m_FramesInFlight; frame++)
            {
                if(!m_DescriptorSet[frame])
                    continue;

                auto descriptorSet = m_DescriptorSet[frame];
                auto pool          = m_DescriptorPoolCreatedFrom[frame];
                auto device        = VKDevice::GetHandle();

                DeletionQueue& deletionQueue = VKRenderer::GetCurrentDeletionQueue();
                deletionQueue.PushFunction([descriptorSet, pool, device]
                                           { vkFreeDescriptorSets(device, pool, 1, &descriptorSet); });
            }

            for(auto bufferData : m_DescriptorData)
            {
                bufferData.LocalStorage.Release();
            }

            g_DescriptorSetCount -= 3;
        }

        void VKDescriptorSet::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        DescriptorSet* VKDescriptorSet::CreateFuncVulkan(const DescriptorDesc& descriptorDesc)
        {
            return new VKDescriptorSet(descriptorDesc);
        }

        void TransitionImageToCorrectLayout(Texture* texture, CommandBuffer* cmdBuffer)
        {
            if(!texture)
                return;

            auto commandBuffer = cmdBuffer ? cmdBuffer : Renderer::GetMainSwapChain()->GetCurrentCommandBuffer();
            if(texture->GetType() == TextureType::COLOUR)
            {
                if(((VKTexture2D*)texture)->GetImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    ((VKTexture2D*)texture)->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VKCommandBuffer*)commandBuffer);
                }
            }
            else if(texture->GetType() == TextureType::CUBE)
            {
                if(((VKTextureCube*)texture)->GetImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                {
                    ((VKTextureCube*)texture)->TransitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, (VKCommandBuffer*)commandBuffer);
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

        void VKDescriptorSet::Update(CommandBuffer* cmdBuffer)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Dynamic                 = false;
            int descriptorWritesCount = 0;
            uint32_t currentFrame     = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            for(auto& bufferInfo : m_DescriptorData)
            {
                if(bufferInfo.valid && !bufferInfo.Desc.m_Members.Empty() && bufferInfo.HasUpdated[currentFrame])
                {
                    m_UniformBuffers[currentFrame][bufferInfo.Binding]->SetData(bufferInfo.LocalStorage.Data);
                    bufferInfo.HasUpdated[currentFrame] = false;
                }
            }

            if(m_DescriptorDirty[currentFrame] || !m_DescriptorUpdated[currentFrame])
            {
                m_DescriptorDirty[currentFrame] = false;
                uint32_t imageIndex             = 0;
                uint32_t index                  = 0;

                ArenaTemp scratch                         = ScratchBegin(nullptr, 0);
                VkDescriptorBufferInfo* bufferInfos       = PushArrayNoZero(scratch.arena, VkDescriptorBufferInfo, 32);
                VkDescriptorImageInfo* imageInfos         = PushArrayNoZero(scratch.arena, VkDescriptorImageInfo, 32);
                VkWriteDescriptorSet* writeDescriptorSets = PushArrayNoZero(scratch.arena, VkWriteDescriptorSet, 32);

                for(auto& imageInfo : m_DescriptorData)
                {
                    if(imageInfo.valid && imageInfo.Desc.type == DescriptorType::IMAGE_SAMPLER && (imageInfo.Desc.texture || imageInfo.Desc.textures))
                    {
                        if(imageInfo.Desc.textureCount == 1)
                        {
                            if(imageInfo.Desc.texture)
                            {
                                TransitionImageToCorrectLayout(imageInfo.Desc.texture, cmdBuffer);

                                VkDescriptorImageInfo& des         = *static_cast<VkDescriptorImageInfo*>(imageInfo.Desc.texture->GetDescriptorInfo());
                                imageInfos[imageIndex].imageLayout = des.imageLayout;
                                imageInfos[imageIndex].imageView   = des.imageView;
                                imageInfos[imageIndex].sampler     = des.sampler;
                            }
                        }
                        else
                        {
                            if(imageInfo.Desc.textures)
                            {
                                for(uint32_t i = 0; i < imageInfo.Desc.textureCount; i++)
                                {
                                    TransitionImageToCorrectLayout(imageInfo.Desc.textures[i], cmdBuffer);

                                    VkDescriptorImageInfo& des             = *static_cast<VkDescriptorImageInfo*>(imageInfo.Desc.textures[i]->GetDescriptorInfo());
                                    imageInfos[i + imageIndex].imageLayout = des.imageLayout;
                                    imageInfos[i + imageIndex].imageView   = des.imageView;
                                    imageInfos[i + imageIndex].sampler     = des.sampler;
                                }
                            }
                        }

                        VkWriteDescriptorSet writeDescriptorSet = {};
                        writeDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet               = m_DescriptorSet[currentFrame];
                        writeDescriptorSet.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        writeDescriptorSet.dstBinding           = imageInfo.Desc.binding;
                        writeDescriptorSet.pImageInfo           = &imageInfos[imageIndex];
                        writeDescriptorSet.descriptorCount      = imageInfo.Desc.textureCount;

                        writeDescriptorSets[descriptorWritesCount] = writeDescriptorSet;
                        imageIndex++;
                        descriptorWritesCount++;
                    }
                    else if(imageInfo.Desc.type == DescriptorType::IMAGE_STORAGE && imageInfo.Desc.texture)
                    {
                        if(imageInfo.Desc.texture)
                        {
                            ((VKTexture2D*)imageInfo.Desc.texture)->TransitionImage(VK_IMAGE_LAYOUT_GENERAL);

                            VkDescriptorImageInfo& des         = *static_cast<VkDescriptorImageInfo*>(imageInfo.Desc.texture->GetDescriptorInfo());
                            imageInfos[imageIndex].imageLayout = des.imageLayout;
                            imageInfos[imageIndex].imageView   = imageInfo.Desc.mipLevel > 0 ? ((VKTexture2D*)imageInfo.Desc.texture)->GetMipImageView(imageInfo.Desc.mipLevel) : des.imageView;
                            imageInfos[imageIndex].sampler     = des.sampler;
                        }

                        VkWriteDescriptorSet writeDescriptorSet = {};
                        writeDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet               = m_DescriptorSet[currentFrame];
                        writeDescriptorSet.descriptorType       = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        writeDescriptorSet.dstBinding           = imageInfo.Desc.binding;
                        writeDescriptorSet.pImageInfo           = &imageInfos[imageIndex];
                        writeDescriptorSet.descriptorCount      = imageInfo.Desc.textureCount;

                        writeDescriptorSets[descriptorWritesCount] = writeDescriptorSet;
                        imageIndex++;
                        descriptorWritesCount++;
                    }

                    else if(imageInfo.Desc.type == DescriptorType::UNIFORM_BUFFER)
                    {
                        VKUniformBuffer* vkUniformBuffer = m_UniformBuffers[currentFrame][imageInfo.Desc.binding].As<VKUniformBuffer>().get();
                        bufferInfos[index].buffer        = *vkUniformBuffer->GetBuffer();
                        bufferInfos[index].offset        = imageInfo.Desc.offset;
                        bufferInfos[index].range         = imageInfo.Desc.size;

                        VkWriteDescriptorSet writeDescriptorSet = {};
                        writeDescriptorSet.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet               = m_DescriptorSet[currentFrame];
                        writeDescriptorSet.descriptorType       = VKUtilities::DescriptorTypeToVK(imageInfo.Desc.type);
                        writeDescriptorSet.dstBinding           = imageInfo.Desc.binding;
                        writeDescriptorSet.pBufferInfo          = &bufferInfos[index];
                        writeDescriptorSet.descriptorCount      = 1;

                        writeDescriptorSets[descriptorWritesCount] = writeDescriptorSet;
                        index++;
                        descriptorWritesCount++;

                        if(imageInfo.Desc.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                            m_Dynamic = true;
                    }
                }

                vkUpdateDescriptorSets(VKDevice::Get().GetDevice(), descriptorWritesCount,
                                       writeDescriptorSets, 0, nullptr);

                ScratchEnd(scratch);
                m_DescriptorUpdated[currentFrame] = true;
            }
        }

        void VKDescriptorSet::TransitionImages(CommandBuffer* commandBuffer)
        {
            for(auto& imageInfo : m_DescriptorData)
            {
                if(imageInfo.valid && (imageInfo.Desc.type == DescriptorType::IMAGE_SAMPLER || imageInfo.Desc.type == DescriptorType::IMAGE_STORAGE) && (imageInfo.Desc.texture || imageInfo.Desc.textures))
                {
                    if(imageInfo.Desc.textureCount == 1)
                    {
                        if(imageInfo.Desc.texture)
                        {
                            if(imageInfo.Desc.type == DescriptorType::IMAGE_STORAGE)
                                ((VKTexture2D*)imageInfo.Desc.texture)->TransitionImage(VK_IMAGE_LAYOUT_GENERAL, (VKCommandBuffer*)commandBuffer);
                            else
                                TransitionImageToCorrectLayout(imageInfo.Desc.texture, commandBuffer);
                        }
                    }
                }
            }
        }

        void VKDescriptorSet::SetTexture(const std::string& name, Texture* texture, uint32_t mipIndex, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();

            for(auto& descriptor : m_DescriptorData)
            {
                if(descriptor.valid && (descriptor.Desc.type == DescriptorType::IMAGE_SAMPLER || descriptor.Desc.type == DescriptorType::IMAGE_STORAGE) && descriptor.Desc.name == name)
                {
                    descriptor.Desc.texture      = texture;
                    descriptor.Desc.textureType  = textureType;
                    descriptor.Desc.textureCount = texture ? 1 : 0;
                    descriptor.Desc.mipLevel     = mipIndex;

                    m_DescriptorDirty[0] = true;
                    m_DescriptorDirty[1] = true;
                    m_DescriptorDirty[2] = true;
                }
            }
        }

        void VKDescriptorSet::SetTexture(const std::string& name, Texture** texture, uint32_t textureCount, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();

            for(auto& descriptor : m_DescriptorData)
            {
                if(descriptor.valid && (descriptor.Desc.type == DescriptorType::IMAGE_SAMPLER || descriptor.Desc.type == DescriptorType::IMAGE_STORAGE) && descriptor.Desc.name == name)
                {
                    descriptor.Desc.textureCount = textureCount;
                    descriptor.Desc.textures     = texture;
                    descriptor.Desc.textureType  = textureType;

                    m_DescriptorDirty[0] = true;
                    m_DescriptorDirty[1] = true;
                    m_DescriptorDirty[2] = true;
                }
            }
        }

        void VKDescriptorSet::SetBuffer(const std::string& name, UniformBuffer* buffer)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
#if 0
            for (auto& descriptor : m_Descriptors[currentFrame].descriptors)
            {
                if (descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
                {
                    descriptor.buffer = buffer;
                }
            }
#endif
        }

        Buffer* VKDescriptorSet::GetUniformBufferLocalData(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = nullptr;
            for(u8 i = 0; i < DESCRIPTOR_MAX_DESCRIPTORS; i++)
            {
                if(!m_DescriptorData[i].Desc.m_Members.Empty() && m_DescriptorData[i].Desc.name == name)
                {
                    lBufferInfo = &m_DescriptorData[i];
                }
            }
            if(lBufferInfo)
            {
                lBufferInfo->HasUpdated[0] = true;
                lBufferInfo->HasUpdated[1] = true;
                lBufferInfo->HasUpdated[2] = true;

                return &lBufferInfo->LocalStorage;
            }

            return nullptr;
        }

        Graphics::UniformBuffer* VKDescriptorSet::GetUniformBuffer(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            // for(auto& buffers : m_UniformBuffers[currentFrame])
            //{
            // if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
            //{
            // return descriptor.buffer;
            // }
            // }

            LWARN("Buffer not found %s", name.c_str());
            return nullptr;
        }

        void VKDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data)
        {
            LUMOS_PROFILE_FUNCTION();
            DescriptorData* lBufferInfo = nullptr;
            for(u8 i = 0; i < DESCRIPTOR_MAX_DESCRIPTORS; i++)
            {
                if(!m_DescriptorData[i].Desc.m_Members.Empty() && m_DescriptorData[i].Desc.name == bufferName)
                {
                    lBufferInfo = &m_DescriptorData[i];
                }
            }
            if(lBufferInfo)
            {
                for(auto& member : lBufferInfo->Desc.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        lBufferInfo->LocalStorage.Write(data, member.size, member.offset);
                        lBufferInfo->HasUpdated[0] = true;
                        lBufferInfo->HasUpdated[1] = true;
                        lBufferInfo->HasUpdated[2] = true;
                        return;
                    }
                }
            }

            LWARN("Uniform not found %s.%s", bufferName.c_str(), uniformName.c_str());
        }

        void VKDescriptorSet::SetUniform(const std::string& bufferName, const std::string& uniformName, void* data, uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = nullptr;
            for(u8 i = 0; i < DESCRIPTOR_MAX_DESCRIPTORS; i++)
            {
                if(!m_DescriptorData[i].Desc.m_Members.Empty() && m_DescriptorData[i].Desc.name == bufferName)
                {
                    lBufferInfo = &m_DescriptorData[i];
                }
            }
            if(lBufferInfo)
            {
                for(auto& member : lBufferInfo->Desc.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        lBufferInfo->LocalStorage.Write(data, size, member.offset);
                        lBufferInfo->HasUpdated[0] = true;
                        lBufferInfo->HasUpdated[1] = true;
                        lBufferInfo->HasUpdated[2] = true;
                        return;
                    }
                }
            }

            LWARN("Uniform not found %s.%s", bufferName.c_str(), uniformName.c_str());
        }

        void VKDescriptorSet::SetUniformBufferData(const std::string& bufferName, void* data)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = nullptr;
            for(u8 i = 0; i < DESCRIPTOR_MAX_DESCRIPTORS; i++)
            {
                if(!m_DescriptorData[i].Desc.m_Members.Empty() && m_DescriptorData[i].Desc.name == bufferName)
                {
                    lBufferInfo = &m_DescriptorData[i];
                }
            }
            if(lBufferInfo)
            {
                lBufferInfo->LocalStorage.Write(data, lBufferInfo->LocalStorage.GetSize(), 0);
                lBufferInfo->HasUpdated[0] = true;
                lBufferInfo->HasUpdated[1] = true;
                lBufferInfo->HasUpdated[2] = true;
                return;
            }

            LWARN("Uniform not found %s", bufferName.c_str());
        }

        void VKDescriptorSet::SetUniformDynamic(const std::string& bufferName, uint32_t size)
        {
            DescriptorData* lBufferInfo = nullptr;
            for(u8 i = 0; i < DESCRIPTOR_MAX_DESCRIPTORS; i++)
            {
                if(!m_DescriptorData[i].Desc.m_Members.Empty() && m_DescriptorData[i].Desc.name == bufferName)
                {
                    lBufferInfo = &m_DescriptorData[i];
                }
            }

            if(lBufferInfo)
            {
                lBufferInfo->LocalStorage.Allocate(size);
                for(auto& member : lBufferInfo->Desc.m_Members)
                {
                    member.size = size;
                }
            }
        }

        void VKDescriptorSet::SetTexture(u8 binding, Texture* texture, uint32_t mipIndex, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& descriptor = m_DescriptorData[binding];
            if((descriptor.Desc.type == DescriptorType::IMAGE_SAMPLER || descriptor.Desc.type == DescriptorType::IMAGE_STORAGE) && texture)
            {
                descriptor.Desc.texture      = texture;
                descriptor.Desc.textureType  = textureType;
                descriptor.Desc.textureCount = texture ? 1 : 0;
                descriptor.Desc.mipLevel     = mipIndex;

                m_DescriptorDirty[0] = true;
                m_DescriptorDirty[1] = true;
                m_DescriptorDirty[2] = true;
            }
        }

        void VKDescriptorSet::SetTexture(u8 binding, Texture** texture, uint32_t textureCount, TextureType textureType)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& descriptor = m_DescriptorData[binding];
            if((descriptor.Desc.type == DescriptorType::IMAGE_SAMPLER || descriptor.Desc.type == DescriptorType::IMAGE_STORAGE) && texture)
            {
                descriptor.Desc.textureCount = textureCount;
                descriptor.Desc.textures     = texture;
                descriptor.Desc.textureType  = textureType;

                m_DescriptorDirty[0] = true;
                m_DescriptorDirty[1] = true;
                m_DescriptorDirty[2] = true;
            }
        }

        void VKDescriptorSet::SetBuffer(u8 binding, UniformBuffer* buffer)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame                 = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();
            m_DescriptorData[binding].Desc.buffer = buffer;
            LWARN("Check");
        }

        Buffer* VKDescriptorSet::GetUniformBufferLocalData(u8 binding)
        {
            LUMOS_PROFILE_FUNCTION();
            DescriptorData* lBufferInfo = &m_DescriptorData[binding];
            if(lBufferInfo)
            {
                lBufferInfo->HasUpdated[0] = true;
                lBufferInfo->HasUpdated[1] = true;
                lBufferInfo->HasUpdated[2] = true;

                return &lBufferInfo->LocalStorage;
            }

            return nullptr;
        }

        Graphics::UniformBuffer* VKDescriptorSet::GetUniformBuffer(u8 binding)
        {
            LUMOS_PROFILE_FUNCTION();
            uint32_t currentFrame = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            // for(auto& buffers : m_UniformBuffers[currentFrame])
            //{
            // if(descriptor.type == DescriptorType::UNIFORM_BUFFER && descriptor.name == name)
            //{
            // return descriptor.buffer;
            // }
            // }

            LWARN("Buffer not found %d", int(binding));
            return nullptr;
        }

        void VKDescriptorSet::SetUniform(u8 binding, const std::string& uniformName, void* data)
        {
            LUMOS_PROFILE_FUNCTION();
            DescriptorData* lBufferInfo = &m_DescriptorData[binding];
            if(lBufferInfo)
            {
                for(auto& member : lBufferInfo->Desc.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        lBufferInfo->LocalStorage.Write(data, member.size, member.offset);
                        lBufferInfo->HasUpdated[0] = true;
                        lBufferInfo->HasUpdated[1] = true;
                        lBufferInfo->HasUpdated[2] = true;
                        return;
                    }
                }
            }

            LWARN("Uniform not found %d.%s", int(binding), uniformName.c_str());
        }

        void VKDescriptorSet::SetUniform(u8 binding, const std::string& uniformName, void* data, uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = &m_DescriptorData[binding];
            if(lBufferInfo)
            {
                for(auto& member : lBufferInfo->Desc.m_Members)
                {
                    if(member.name == uniformName)
                    {
                        lBufferInfo->LocalStorage.Write(data, size, member.offset);
                        lBufferInfo->HasUpdated[0] = true;
                        lBufferInfo->HasUpdated[1] = true;
                        lBufferInfo->HasUpdated[2] = true;
                        return;
                    }
                }
            }

            LWARN("Uniform not found %d.%s", int(binding), uniformName.c_str());
        }

        void VKDescriptorSet::SetUniformBufferData(u8 binding, void* data)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = &m_DescriptorData[binding];
            if(lBufferInfo)
            {
                lBufferInfo->LocalStorage.Write(data, lBufferInfo->LocalStorage.GetSize(), 0);
                lBufferInfo->HasUpdated[0] = true;
                lBufferInfo->HasUpdated[1] = true;
                lBufferInfo->HasUpdated[2] = true;
                return;
            }

            LWARN("Uniform not found %d", int(binding));
        }

        void VKDescriptorSet::SetUniformBufferData(u8 binding, void* data, float size)
        {
            LUMOS_PROFILE_FUNCTION();

            DescriptorData* lBufferInfo = &m_DescriptorData[binding];
            if(lBufferInfo)
            {
                ASSERT(size <= lBufferInfo->LocalStorage.GetSize());
                lBufferInfo->LocalStorage.Write(data, size, 0);
                lBufferInfo->HasUpdated[0] = true;
                lBufferInfo->HasUpdated[1] = true;
                lBufferInfo->HasUpdated[2] = true;
                return;
            }

            LWARN("Uniform not found %d", int(binding));
        }
    }
}
