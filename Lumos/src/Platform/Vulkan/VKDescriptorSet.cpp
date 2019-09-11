#include "lmpch.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture.h"
#include "VKDevice.h"

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

			Graphics::VKDevice::Instance()->GetDevice().allocateDescriptorSets(&descriptorSetAllocateInfo, &m_DescriptorSet);
		}

		VKDescriptorSet::~VKDescriptorSet()
		{
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
            
            std::vector<vk::DescriptorImageInfo*> allocatedArrays;
            vk::DescriptorBufferInfo* buffersInfo = nullptr;
            
            m_Dynamic = false;
            
            if(imageInfos != nullptr)
            {
                for (auto& imageInfo : *imageInfos)
                {
                    vk::DescriptorImageInfo* imageInfosPtr = lmnew vk::DescriptorImageInfo[imageInfo.count];
                    allocatedArrays.emplace_back(imageInfosPtr);

					for (int i = 0; i < imageInfo.count; i++)
					{
						vk::DescriptorImageInfo des = *static_cast<vk::DescriptorImageInfo*>(imageInfo.texture[i]->GetHandle());
						imageInfosPtr[i].imageLayout = des.imageLayout;
						imageInfosPtr[i].imageView = des.imageView;
						imageInfosPtr[i].sampler = des.sampler;
					}

					vk::WriteDescriptorSet writeDescriptorSet{};
					writeDescriptorSet.dstSet = m_DescriptorSet;
					writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
					writeDescriptorSet.dstBinding = imageInfo.binding;
					writeDescriptorSet.pImageInfo = imageInfosPtr;
					writeDescriptorSet.descriptorCount = imageInfo.count;

                    descriptorWrites.push_back(writeDescriptorSet);
                }
            }
      
            if(bufferInfos != nullptr)
            {
                buffersInfo = lmnew vk::DescriptorBufferInfo[bufferInfos->size()];

                int index = 0;
                
                for (auto& bufferInfo : *bufferInfos)
                {
                    buffersInfo[index].buffer = *dynamic_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
                    buffersInfo[index].offset = bufferInfo.offset;
                    buffersInfo[index].range = bufferInfo.size;
    
                    vk::WriteDescriptorSet writeDescriptorSet{};
                    writeDescriptorSet.dstSet = m_DescriptorSet;
                    writeDescriptorSet.descriptorType = VKTools::DescriptorTypeToVK(bufferInfo.type);
                    writeDescriptorSet.dstBinding = bufferInfo.binding;
                    writeDescriptorSet.pBufferInfo = &buffersInfo[index];
                    writeDescriptorSet.descriptorCount = 1;
    
                    descriptorWrites.emplace_back(writeDescriptorSet);
                    index++;
    
                    if (bufferInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                        m_Dynamic = true;
                }
            }
            
            VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
            
            delete[] buffersInfo;
            
            for (auto& allocatedArray : allocatedArrays)
            {
                delete[] allocatedArray;
            }
        }
    }
}
