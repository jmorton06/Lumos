#include "Precompiled.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture.h"
#include "VKDevice.h"

#define MAX_BUFFER_INFOS 32
#define MAX_IMAGE_INFOS 32
#define MAX_WRITE_DESCTIPTORS 32

namespace Lumos
{
    namespace Graphics
    {
        VKDescriptorSet::VKDescriptorSet(const DescriptorInfo& info)
        {
            LUMOS_PROFILE_FUNCTION();
            VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
            descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            descriptorSetAllocateInfo.descriptorPool = static_cast<Graphics::VKPipeline*>(info.pipeline)->GetDescriptorPool();
            descriptorSetAllocateInfo.pSetLayouts = static_cast<Graphics::VKPipeline*>(info.pipeline)->GetDescriptorLayout(info.layoutIndex);
            descriptorSetAllocateInfo.descriptorSetCount = info.count;
            descriptorSetAllocateInfo.pNext = nullptr;

            VK_CHECK_RESULT(vkAllocateDescriptorSets(VKDevice::GetHandle(), &descriptorSetAllocateInfo, &m_DescriptorSet));

            m_BufferInfoPool = new VkDescriptorBufferInfo[MAX_BUFFER_INFOS];
            m_ImageInfoPool = new VkDescriptorImageInfo[MAX_IMAGE_INFOS];
            m_WriteDescriptorSetPool = new VkWriteDescriptorSet[MAX_WRITE_DESCTIPTORS];
        }

        VKDescriptorSet::~VKDescriptorSet()
        {
            delete[] m_BufferInfoPool;
            delete[] m_ImageInfoPool;
            delete[] m_WriteDescriptorSetPool;
        }

        void VKDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            UpdateInternal(nullptr, &bufferInfos);
        }

        void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            UpdateInternal(&imageInfos, nullptr);
        }

        void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            UpdateInternal(&imageInfos, &bufferInfos);
        }

        void VKDescriptorSet::MakeDefault()
        {
            CreateFunc = CreateFuncVulkan;
        }

        DescriptorSet* VKDescriptorSet::CreateFuncVulkan(const DescriptorInfo& info)
        {
            return new VKDescriptorSet(info);
        }

        void VKDescriptorSet::UpdateInternal(std::vector<ImageInfo>* imageInfos, std::vector<BufferInfo>* bufferInfos)
        {
            LUMOS_PROFILE_FUNCTION();
            m_Dynamic = false;
            int descriptorWritesCount = 0;

            if(imageInfos != nullptr)
            {
                int imageIndex = 0;

                for(auto& imageInfo : *imageInfos)
                {
                    if(imageInfo.count == 1)
                    {
                        VkDescriptorImageInfo& des = *static_cast<VkDescriptorImageInfo*>(imageInfo.texture->GetHandle());
                        m_ImageInfoPool[imageIndex].imageLayout = des.imageLayout;
                        m_ImageInfoPool[imageIndex].imageView = des.imageView;
                        m_ImageInfoPool[imageIndex].sampler = des.sampler;
                    }
                    else
                    {
                        for(int i = 0; i < imageInfo.count; i++)
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
                    writeDescriptorSet.descriptorCount = imageInfo.count;

                    m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                    imageIndex++;
                    descriptorWritesCount++;
                }
            }

            if(bufferInfos != nullptr)
            {
                int index = 0;

                for(auto& bufferInfo : *bufferInfos)
                {
                    m_BufferInfoPool[index].buffer = *dynamic_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
                    m_BufferInfoPool[index].offset = bufferInfo.offset;
                    m_BufferInfoPool[index].range = bufferInfo.size;

                    VkWriteDescriptorSet writeDescriptorSet {};
                    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    writeDescriptorSet.dstSet = m_DescriptorSet;
                    writeDescriptorSet.descriptorType = VKTools::DescriptorTypeToVK(bufferInfo.type);
                    writeDescriptorSet.dstBinding = bufferInfo.binding;
                    writeDescriptorSet.pBufferInfo = &m_BufferInfoPool[index];
                    writeDescriptorSet.descriptorCount = 1;

                    m_WriteDescriptorSetPool[descriptorWritesCount] = writeDescriptorSet;
                    index++;
                    descriptorWritesCount++;

                    if(bufferInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                        m_Dynamic = true;
                }
            }

            vkUpdateDescriptorSets(VKDevice::Get().GetDevice(), descriptorWritesCount,
                m_WriteDescriptorSetPool, 0, nullptr);
        }
    }
}
