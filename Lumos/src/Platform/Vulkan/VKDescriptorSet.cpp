#include "lmpch.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture.h"
#include "VKDevice.h"

#define MAX_BUFFER_INFOS 32
#define MAX_IMAGE_INFOS 32

namespace Lumos
{
	namespace Graphics
	{
		VKDescriptorSet::VKDescriptorSet(const DescriptorInfo& info)
		{
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.descriptorPool = static_cast<Graphics::VKPipeline*>(info.pipeline)->GetDescriptorPool();
			descriptorSetAllocateInfo.pSetLayouts = static_cast<Graphics::VKPipeline*>(info.pipeline)->GetDescriptorLayout(info.layoutIndex);
			descriptorSetAllocateInfo.descriptorSetCount = info.count;

			vk::Result result = Graphics::VKDevice::Instance()->GetDevice().allocateDescriptorSets(&descriptorSetAllocateInfo, &m_DescriptorSet);

			if (result != vk::Result::eSuccess)
				LUMOS_ASSERT(false, "Failed to allocate Descriptor Set");

			m_BufferInfoPool = lmnew vk::DescriptorBufferInfo[MAX_BUFFER_INFOS];
			m_ImageInfoPool = lmnew vk::DescriptorImageInfo[MAX_IMAGE_INFOS];
		}

		VKDescriptorSet::~VKDescriptorSet()
		{
			lmdel[] m_BufferInfoPool;
			lmdel[] m_ImageInfoPool;
		}

		void VKDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
		{
            UpdateInternal(nullptr, &bufferInfos);
		}

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
		{
             UpdateInternal(&imageInfos, nullptr);
		}

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
		{
            UpdateInternal(&imageInfos, &bufferInfos);
		}

		void VKDescriptorSet::SetPushConstants(std::vector<PushConstant>& pushConstants)
		{
			m_PushConstants.clear();
			for (auto& pushConstant : pushConstants)
			{
				m_PushConstants.push_back(pushConstant);
			}
		}

		void VKDescriptorSet::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
		}

		DescriptorSet* VKDescriptorSet::CreateFuncVulkan(const DescriptorInfo& info)
		{
			return lmnew VKDescriptorSet(info);
		}
    
        void VKDescriptorSet::UpdateInternal(std::vector<ImageInfo>* imageInfos, std::vector<BufferInfo>* bufferInfos)
        {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;
            
            m_Dynamic = false;
            
            if(imageInfos != nullptr)
            {
				int imageIndex = 0;

                for (auto& imageInfo : *imageInfos)
                {
					for (int i = 0; i < imageInfo.count; i++)
					{
						vk::DescriptorImageInfo des = *static_cast<vk::DescriptorImageInfo*>(imageInfo.texture[i]->GetHandle());
						m_ImageInfoPool[i + imageIndex].imageLayout = des.imageLayout;
						m_ImageInfoPool[i + imageIndex].imageView = des.imageView;
						m_ImageInfoPool[i + imageIndex].sampler = des.sampler;
					}

					vk::WriteDescriptorSet writeDescriptorSet{};
					writeDescriptorSet.dstSet = m_DescriptorSet;
					writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
					writeDescriptorSet.dstBinding = imageInfo.binding;
					writeDescriptorSet.pImageInfo = &m_ImageInfoPool[imageIndex];
					writeDescriptorSet.descriptorCount = imageInfo.count;

                    descriptorWrites.push_back(writeDescriptorSet);
					imageIndex++;
                }
            }
      
            if(bufferInfos != nullptr)
            {
                int index = 0;
                
                for (auto& bufferInfo : *bufferInfos)
                {
					m_BufferInfoPool[index].buffer = *dynamic_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
					m_BufferInfoPool[index].offset = bufferInfo.offset;
					m_BufferInfoPool[index].range = bufferInfo.size;
    
                    vk::WriteDescriptorSet writeDescriptorSet{};
                    writeDescriptorSet.dstSet = m_DescriptorSet;
                    writeDescriptorSet.descriptorType = VKTools::DescriptorTypeToVK(bufferInfo.type);
                    writeDescriptorSet.dstBinding = bufferInfo.binding;
                    writeDescriptorSet.pBufferInfo = &m_BufferInfoPool[index];
                    writeDescriptorSet.descriptorCount = 1;
    
                    descriptorWrites.emplace_back(writeDescriptorSet);
                    index++;
    
                    if (bufferInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                        m_Dynamic = true;
                }
            }
            
            VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}
