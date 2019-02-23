#include "LM.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture2D.h"

#include "VKTextureCube.h"
#include "VKTextureDepth.h"
#include "VKTextureDepthArray.h"

namespace Lumos
{
	namespace graphics
	{
		VKDescriptorSet::VKDescriptorSet(api::DescriptorInfo info)
		{
			vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{};
			descriptorSetAllocateInfo.descriptorPool = static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorPool();
			descriptorSetAllocateInfo.pSetLayouts = static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorLayout(info.layoutIndex);
			descriptorSetAllocateInfo.descriptorSetCount = 1;

			graphics::VKDevice::Instance()->GetDevice().allocateDescriptorSets(&descriptorSetAllocateInfo, &m_DescriptorSet);
		}

		VKDescriptorSet::~VKDescriptorSet()
		{
		}

		vk::WriteDescriptorSet writeDescriptorSet(
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

		vk::WriteDescriptorSet writeDescriptorSet(
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

		void VKDescriptorSet::Update(std::vector<api::BufferInfo>& bufferInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;
			vk::DescriptorBufferInfo* buffersInfo = new vk::DescriptorBufferInfo[bufferInfos.size()];

			m_Dynamic = false;

			int index = 0;

			for (auto& bufferInfo : bufferInfos)
			{
				buffersInfo[index].buffer = *static_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
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

				if (bufferInfo.type == api::DescriptorType::UNIFORM_BUFFER_DYNAMIC)
					m_Dynamic = true;
			}

			VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),0,nullptr);

			delete[] buffersInfo;
		}

		void VKDescriptorSet::Update(std::vector<api::ImageInfo>& imageInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;

			m_Dynamic = false;

			for (auto& imageInfo : imageInfos)
			{
				switch(imageInfo.type)
				{
				case TextureType::COLOUR : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTexture2D*>(imageInfo.texture)->GetDescriptor())); break;
				case TextureType::DEPTH : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureDepth*>(imageInfo.texture)->GetDescriptor())); break;
				case TextureType::DEPTHARRAY : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureDepthArray*>(imageInfo.texture)->GetDescriptor())); break;
				case TextureType::CUBE : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureCube*>(imageInfo.texture)->GetDescriptor())); break;
					default : LUMOS_CORE_ERROR("Unsupported Texture Type",""); break;
				}
			}
			VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		}

		void VKDescriptorSet::Update(std::vector<api::ImageInfo>& imageInfos, std::vector<api::BufferInfo>& bufferInfos)
		{
			std::vector<vk::WriteDescriptorSet> descriptorWrites;

			m_Dynamic = false;

			for(auto& imageInfo : imageInfos)
			{
				switch(imageInfo.type)
				{
					case TextureType::COLOUR : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTexture2D*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTH : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureDepth*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTHARRAY : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureDepthArray*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::CUBE : descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, vk::DescriptorType::eCombinedImageSampler, imageInfo.binding, static_cast<VKTextureCube*>(imageInfo.texture)->GetDescriptor())); break;
					default : LUMOS_CORE_ERROR("Unsupported Texture Type",""); break;
				}
			}

			for (auto& bufferInfo : bufferInfos)
			{
				vk::DescriptorBufferInfo info = {};
				info.buffer = *static_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
				info.offset = bufferInfo.offset;
				info.range = bufferInfo.size;

				if (bufferInfo.type == api::DescriptorType::UNIFORM_BUFFER_DYNAMIC)
					m_Dynamic = true;

				descriptorWrites.push_back(writeDescriptorSet(m_DescriptorSet, VKTools::DescriptorTypeToVK(bufferInfo.type), bufferInfo.binding, &info));
			}

			VKDevice::Instance()->GetDevice().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		void VKDescriptorSet::SetPushConstants(std::vector<api::PushConstant>& pushConstants)
		{
			m_PushConstants.clear();
			for (auto& pushConstant : pushConstants)
			{
				m_PushConstants.push_back(pushConstant);
			}
		}
	}
}
