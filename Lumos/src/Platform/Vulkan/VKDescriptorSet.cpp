#include "LM.h"
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
		VKDescriptorSet::VKDescriptorSet(DescriptorInfo info)
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

		vk::WriteDescriptorSet GetWriteDescriptorSet(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorImageInfo *imageInfo,
			uint32_t descriptorCount = 1)
		{
			vk::WriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pImageInfo = imageInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		vk::WriteDescriptorSet GetWriteDescriptorSet(
			vk::DescriptorSet dstSet,
			vk::DescriptorType type,
			uint32_t binding,
			vk::DescriptorBufferInfo *bufferInfo,
			uint32_t descriptorCount = 1)
		{
			vk::WriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.dstSet = dstSet;
			writeDescriptorSet.descriptorType = type;
			writeDescriptorSet.dstBinding = binding;
			writeDescriptorSet.pBufferInfo = bufferInfo;
			writeDescriptorSet.descriptorCount = descriptorCount;
			return writeDescriptorSet;
		}

		vk::WriteDescriptorSet VKDescriptorSet::ImageInfoToVK(ImageInfo& imageInfo)
		{
            std::vector<vk::DescriptorImageInfo> imageInfos(imageInfo.count);
            
			for (int i = 0; i < imageInfo.count; i++)
			{
				vk::DescriptorImageInfo des;
				switch (imageInfo.type)
				{
				case TextureType::COLOUR:
					des = static_cast<VKTexture2D*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::DEPTH:
					des = static_cast<VKTextureCube*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::DEPTHARRAY: 
					des = static_cast<VKTextureDepth*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::CUBE:
					des = static_cast<VKTextureDepthArray*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				default: LUMOS_CORE_ERROR("Unsupported Texture Type"); des = vk::DescriptorImageInfo(); break;
				}
			}

			vk::WriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.dstSet = m_DescriptorSet;
			writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			writeDescriptorSet.dstBinding = imageInfo.binding;
			writeDescriptorSet.pImageInfo = imageInfos.data();
			writeDescriptorSet.descriptorCount = imageInfo.count;

			return writeDescriptorSet;
		}

		void VKDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
		{
            std::vector<ImageInfo> temp;
            UpdateInternal(temp, bufferInfos);
		}

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
		{
            std::vector<BufferInfo> temp;
			UpdateInternal(imageInfos, temp);
        }

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
		{
            UpdateInternal(imageInfos, bufferInfos);
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
        
		DescriptorSet* VKDescriptorSet::CreateFuncVulkan(DescriptorInfo info)
        {
            return lmnew VKDescriptorSet(info);
        }
        
        void VKDescriptorSet::UpdateInternal(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
        {
            std::vector<vk::WriteDescriptorSet> descriptorWrites;
            std::vector<std::vector<vk::DescriptorImageInfo>> allocatedArrays;
            
            m_Dynamic = false;
            
            for (auto& imageInfo : imageInfos)
            {
                std::vector<vk::DescriptorImageInfo> descriptorImageInfo(imageInfo.count);
                
                for (int i = 0; i < imageInfo.count; i++)
                {
                    vk::DescriptorImageInfo des;
                    switch (imageInfo.type)
                    {
                        case TextureType::COLOUR:
                            des = static_cast<VKTexture2D*>(imageInfo.texture[i])->GetDescriptor();
                            break;
                        case TextureType::DEPTH:
                            des = static_cast<VKTextureCube*>(imageInfo.texture[i])->GetDescriptor();
                            break;
                        case TextureType::DEPTHARRAY:
                            des = static_cast<VKTextureDepth*>(imageInfo.texture[i])->GetDescriptor();
                            break;
                        case TextureType::CUBE:
                            des = static_cast<VKTextureDepthArray*>(imageInfo.texture[i])->GetDescriptor();
                            break;
                        default: LUMOS_CORE_ERROR("Unsupported Texture Type"); des = vk::DescriptorImageInfo(); break;
                    }
                    
                    descriptorImageInfo[i] = des;
                }
                
                vk::WriteDescriptorSet writeDescriptorSet{};
                writeDescriptorSet.dstSet = m_DescriptorSet;
                writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
                writeDescriptorSet.dstBinding = imageInfo.binding;
                writeDescriptorSet.pImageInfo = descriptorImageInfo.data();
                writeDescriptorSet.descriptorCount = imageInfo.count;
                
                descriptorWrites.emplace_back(writeDescriptorSet);
                
                allocatedArrays.emplace_back(descriptorImageInfo);
            }
            
            for (auto& bufferInfo : bufferInfos)
            {
                vk::DescriptorBufferInfo info = {};
                info.buffer = *dynamic_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
                info.offset = bufferInfo.offset;
                info.range = bufferInfo.size;
                
                if (bufferInfo.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
                    m_Dynamic = true;
                
                descriptorWrites.push_back(GetWriteDescriptorSet(m_DescriptorSet, VKTools::DescriptorTypeToVK(bufferInfo.type), bufferInfo.binding, &info));
            }
            
            VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
	}
}
