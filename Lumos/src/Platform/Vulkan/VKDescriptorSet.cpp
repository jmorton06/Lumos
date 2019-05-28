#include "LM.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture2D.h"
#include "VKDevice.h"
#include "VKTextureCube.h"
#include "VKTextureDepth.h"
#include "VKTextureDepthArray.h"

namespace lumos
{
	namespace graphics
	{
		VKDescriptorSet::VKDescriptorSet(DescriptorInfo info)
		{
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.descriptorPool = static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorPool();
			descriptorSetAllocateInfo.pSetLayouts = static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorLayout(info.layoutIndex);
			descriptorSetAllocateInfo.descriptorSetCount = info.count;

			graphics::VKDevice::Instance()->GetDevice().allocateDescriptorSets(&descriptorSetAllocateInfo, &m_DescriptorSet);
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
			std::vector<vk::DescriptorImageInfo> descriptorInfo;
			for(int i = 0; i < imageInfo.count; i++)
			{
				switch (imageInfo.type)
				{
				case TextureType::COLOUR: descriptorInfo.push_back(*static_cast<VKTexture2D*>(imageInfo.texture[i])->GetDescriptor()); break;
				case TextureType::DEPTH: descriptorInfo.push_back(*static_cast<VKTextureDepth*>(imageInfo.texture[i])->GetDescriptor()); break;
				case TextureType::DEPTHARRAY: descriptorInfo.push_back(*static_cast<VKTextureDepthArray*>(imageInfo.texture[i])->GetDescriptor()); break;
				case TextureType::CUBE: descriptorInfo.push_back(*static_cast<VKTextureCube*>(imageInfo.texture[i])->GetDescriptor()); break;
				default: LUMOS_CORE_ERROR("Unsupported Texture Type"); break;
				}
			}

			vk::WriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.dstSet = m_DescriptorSet;
			writeDescriptorSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
			writeDescriptorSet.dstBinding = imageInfo.binding;
			writeDescriptorSet.pImageInfo = descriptorInfo.data();
			writeDescriptorSet.descriptorCount = imageInfo.count;

			return writeDescriptorSet;
		}

		vk::WriteDescriptorSet VKDescriptorSet::ImageInfoToVK2(ImageInfo& imageInfo,vk::DescriptorImageInfo* imageInfos)
		{
			for (int i = 0; i < imageInfo.count; i++)
			{
				vk::DescriptorImageInfo des;
				switch (imageInfo.type)
				{
				case TextureType::COLOUR:
					des = *static_cast<VKTexture2D*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::DEPTH:
					des = *static_cast<VKTextureDepth*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::DEPTHARRAY: 
					des = *static_cast<VKTextureDepthArray*>(imageInfo.texture[i])->GetDescriptor();
					imageInfos[i].imageLayout = des.imageLayout;
					imageInfos[i].imageView = des.imageView;
					imageInfos[i].sampler = des.sampler;
					break;
				case TextureType::CUBE:
					des = *static_cast<VKTextureCube*>(imageInfo.texture[i])->GetDescriptor();
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
			writeDescriptorSet.pImageInfo = imageInfos;
			writeDescriptorSet.descriptorCount = imageInfo.count;

			return writeDescriptorSet;
		}

		void VKDescriptorSet::Update(std::vector<BufferInfo>& bufferInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			vk::DescriptorBufferInfo* buffersInfo = new vk::DescriptorBufferInfo[bufferInfos.size()];

			m_Dynamic = false;

			int index = 0;

			for (auto& bufferInfo : bufferInfos)
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

			VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),0,nullptr);

			delete[] buffersInfo;
		}

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
            std::vector<vk::DescriptorImageInfo*> allocatedArrays;

			m_Dynamic = false;

			for (auto& imageInfo : imageInfos)
			{
                vk::DescriptorImageInfo* imageInfos = new vk::DescriptorImageInfo[imageInfo.count];
                allocatedArrays.emplace_back(imageInfos);
				descriptorWrites.push_back(ImageInfoToVK2(imageInfo,imageInfos));
			}
            
			VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		
            for (auto& allocatedArray : allocatedArrays)
            {
                delete[] allocatedArray;
            }
        }

		void VKDescriptorSet::Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
            std::vector<vk::DescriptorImageInfo*> allocatedArrays;

			m_Dynamic = false;

			for (auto& imageInfo : imageInfos)
			{
                vk::DescriptorImageInfo* imageInfos = new vk::DescriptorImageInfo[imageInfo.count];
                allocatedArrays.emplace_back(imageInfos);
				descriptorWrites.push_back(ImageInfoToVK2(imageInfo,imageInfos));
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
		
            for (auto& allocatedArray : allocatedArrays)
            {
                delete[] allocatedArray;
            }
        }

		void VKDescriptorSet::SetPushConstants(std::vector<PushConstant>& pushConstants)
		{
			m_PushConstants.clear();
			for (auto& pushConstant : pushConstants)
			{
				m_PushConstants.push_back(pushConstant);
			}
		}
	}
}
