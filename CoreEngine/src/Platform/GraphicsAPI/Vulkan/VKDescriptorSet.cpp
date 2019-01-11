#include "JM.h"
#include "VKDescriptorSet.h"
#include "VKInitialisers.h"
#include "VKPipeline.h"
#include "VKTools.h"
#include "VKUniformBuffer.h"
#include "VKTexture2D.h"

#include "VKTextureCube.h"
#include "VKTextureDepth.h"
#include "VKTextureDepthArray.h"

namespace jm
{
	namespace graphics
	{
		VKDescriptorSet::VKDescriptorSet(api::DescriptorInfo info)
		{
			VkDescriptorSetAllocateInfo allocInfo =
				initializers::descriptorSetAllocateInfo(
					static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorPool(),
					static_cast<graphics::VKPipeline*>(info.pipeline)->GetDescriptorLayout(info.layoutIndex),
					1);

			VK_CHECK_RESULT(vkAllocateDescriptorSets(graphics::VKDevice::Instance()->GetDevice(), &allocInfo, &m_DescriptorSet));
		}

		VKDescriptorSet::~VKDescriptorSet()
		{
		}

		void VKDescriptorSet::Update(std::vector<api::BufferInfo>& bufferInfos)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites;
			VkDescriptorBufferInfo* buffersInfo = new VkDescriptorBufferInfo[bufferInfos.size()];

			m_Dynamic = false;

			int index = 0;

			for (auto& bufferInfo : bufferInfos)
			{
				buffersInfo[index].buffer = *static_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
				buffersInfo[index].offset = bufferInfo.offset;
				buffersInfo[index].range = bufferInfo.size;
				descriptorWrites.emplace_back(initializers::writeDescriptorSet(m_DescriptorSet, VKTools::DescriptorTypeToVK(bufferInfo.type), bufferInfo.binding, &buffersInfo[index]));
				index++;

				if (bufferInfo.type == api::DescriptorType::UNIFORM_BUFFER_DYNAMIC)
					m_Dynamic = true;
			}

			vkUpdateDescriptorSets(VKDevice::Instance()->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(), 0, nullptr);

			delete[] buffersInfo;
		}

		void VKDescriptorSet::Update(std::vector<api::ImageInfo>& imageInfos)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites;

			m_Dynamic = false;

			for (auto& imageInfo : imageInfos)
			{
				switch(imageInfo.type)
				{
					case TextureType::COLOUR : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTexture2D*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTH : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureDepth*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTHARRAY : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureDepthArray*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::CUBE : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureCube*>(imageInfo.texture)->GetDescriptor())); break;
					default : JM_CORE_ERROR("Unsupported Texture Type",""); break;
				}
			}
			vkUpdateDescriptorSets(VKDevice::Instance()->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(), 0, nullptr);
		}

		void VKDescriptorSet::Update(std::vector<api::ImageInfo>& imageInfos, std::vector<api::BufferInfo>& bufferInfos)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites;

			m_Dynamic = false;

			for(auto& imageInfo : imageInfos)
			{
				switch(imageInfo.type)
				{
					case TextureType::COLOUR : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTexture2D*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTH : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureDepth*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::DEPTHARRAY : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureDepthArray*>(imageInfo.texture)->GetDescriptor())); break;
					case TextureType::CUBE : descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo.binding, static_cast<VKTextureCube*>(imageInfo.texture)->GetDescriptor())); break;
					default : JM_CORE_ERROR("Unsupported Texture Type",""); break;
				}
			}

			for (auto& bufferInfo : bufferInfos)
			{
				VkDescriptorBufferInfo info = {};
				info.buffer = *static_cast<VKUniformBuffer*>(bufferInfo.buffer)->GetBuffer();
				info.offset = bufferInfo.offset;
				info.range = bufferInfo.size;

				if (bufferInfo.type == api::DescriptorType::UNIFORM_BUFFER_DYNAMIC)
					m_Dynamic = true;

				descriptorWrites.push_back(initializers::writeDescriptorSet(m_DescriptorSet, VKTools::DescriptorTypeToVK(bufferInfo.type), bufferInfo.binding, &info));
			}

			vkUpdateDescriptorSets(VKDevice::Instance()->GetDevice(), static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(), 0, nullptr);
		}
	}
}
