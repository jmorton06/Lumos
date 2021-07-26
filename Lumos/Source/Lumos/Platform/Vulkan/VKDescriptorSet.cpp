#include "Precompiled.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
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
        VKDescriptorSet::VKDescriptorSet(const DescriptorDesc& info)
        {
            LUMOS_PROFILE_FUNCTION();
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool = VKRenderer::GetRenderer()->GetDescriptorPool();
            descriptorSetAllocateInfo.pSetLayouts = static_cast<Graphics::VKShader*>(info.shader)->GetDescriptorLayout(info.layoutIndex);
            descriptorSetAllocateInfo.descriptorSetCount = info.count;
            descriptorSetAllocateInfo.pNext = nullptr;

            VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDevice::GetHandle(), &descriptorSetAllocateInfo, &m_DescriptorSet));

            m_BufferInfoPool = new VkDescriptorBufferInfo[MAX_BUFFER_INFOS];
            m_ImageInfoPool = new VkDescriptorImageInfo[MAX_IMAGE_INFOS];
            m_WriteDescriptorSetPool = new VkWriteDescriptorSet[MAX_WRITE_DESCTIPTORS];

            m_Shader = info.shader;

            m_Descriptors = m_Shader->GetDescriptorInfo(info.layoutIndex);

            for(auto& bufferInfo : m_Descriptors.descriptors)
            {
                if(bufferInfo.type == DescriptorType::UNIFORM_BUFFER)
                {
                    // bufferInfo.buffer = Graphics::UniformBuffer::Create();
                    // bufferInfo.buffer->Init(bufferInfo.size, nullptr);
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

        DescriptorSet* VKDescriptorSet::CreateFuncVulkan(const DescriptorDesc& info)
        {
            return new VKDescriptorSet(info);
        }

        void VKDescriptorSet::Update(std::vector<Descriptor>& descriptors)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Dynamic = false;
            int descriptorWritesCount = 0;

            {
                int imageIndex = 0;
                int index = 0;

                for(auto& imageInfo : descriptors)
                {

                    if(imageInfo.type == DescriptorType::IMAGE_SAMPLER)
                    {
                        if(imageInfo.textureCount == 1)
                        {
                            VkDescriptorImageInfo& des = *static_cast<VkDescriptorImageInfo*>(imageInfo.texture->GetHandle());
                            m_ImageInfoPool[imageIndex].imageLayout = des.imageLayout;
                            m_ImageInfoPool[imageIndex].imageView = des.imageView;
                            m_ImageInfoPool[imageIndex].sampler = des.sampler;
                        }
                        else
                        {
                            for(int i = 0; i < imageInfo.textureCount; i++)
                            {
                                VkDescriptorImageInfo& des = *static_cast<VkDescriptorImageInfo*>(imageInfo.textures[i]->GetHandle());
                                m_ImageInfoPool[i + imageIndex].imageLayout = des.imageLayout;
                                m_ImageInfoPool[i + imageIndex].imageView = des.imageView;
                                m_ImageInfoPool[i + imageIndex].sampler = des.sampler;
                            }
                        }

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = m_DescriptorSet;
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
                        m_BufferInfoPool[index].buffer = *dynamic_cast<VKUniformBuffer*>(imageInfo.buffer)->GetBuffer();
                        m_BufferInfoPool[index].offset = imageInfo.offset;
                        m_BufferInfoPool[index].range = imageInfo.size;

                        VkWriteDescriptorSet writeDescriptorSet {};
                        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        writeDescriptorSet.dstSet = m_DescriptorSet;
                        writeDescriptorSet.descriptorType = VKTools::DescriptorTypeToVK(imageInfo.type);
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

            vkUpdateDescriptorSets(VKDevice::Get().GetDevice(), descriptorWritesCount,
                m_WriteDescriptorSetPool, 0, nullptr);
        }
    }
}
