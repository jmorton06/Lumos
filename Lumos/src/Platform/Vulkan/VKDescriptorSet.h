#pragma once
#include "Graphics/API/DescriptorSet.h"
#include "VK.h"

namespace lumos
{
	namespace graphics
	{
		class VKDescriptorSet : public DescriptorSet
		{
		public:
			VKDescriptorSet(DescriptorInfo info);
			~VKDescriptorSet();

			vk::DescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

			void Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos) override;
			void Update(std::vector<BufferInfo>& bufferInfos) override;
			void Update(std::vector<ImageInfo>& imageInfos) override;
			void SetPushConstants(std::vector<PushConstant>& pushConstants) override;
			bool GetIsDynamic() const { return m_Dynamic; }

			std::vector<PushConstant> GetPushConstants() const { return m_PushConstants; }

			vk::WriteDescriptorSet ImageInfoToVK(ImageInfo& imageInfo);
			vk::WriteDescriptorSet ImageInfoToVK2(ImageInfo& imageInfo,vk::DescriptorImageInfo* imageInfos);


		private:
			vk::DescriptorSet m_DescriptorSet;
			bool m_Dynamic = false;

			std::vector<PushConstant> m_PushConstants;
		};
	}
}
