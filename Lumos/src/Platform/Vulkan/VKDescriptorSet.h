#pragma once
#include "Graphics/API/DescriptorSet.h"
#include "VK.h"

namespace Lumos
{
	namespace graphics
	{
		class VKDescriptorSet : public api::DescriptorSet
		{
		public:
			VKDescriptorSet(api::DescriptorInfo info);
			~VKDescriptorSet();

			vk::DescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

			void Update(std::vector<api::ImageInfo>& imageInfos, std::vector<api::BufferInfo>& bufferInfos) override;
			void Update(std::vector<api::BufferInfo>& bufferInfos) override;
			void Update(std::vector<api::ImageInfo>& imageInfos) override;
			void SetPushConstants(std::vector<api::PushConstant>& pushConstants) override;
			bool GetIsDynamic() const { return m_Dynamic; }

			std::vector<api::PushConstant> GetPushConstants() const { return m_PushConstants; }

			vk::WriteDescriptorSet ImageInfoToVK(api::ImageInfo& imageInfo);
			vk::WriteDescriptorSet ImageInfoToVK2(api::ImageInfo& imageInfo);


		private:
			vk::DescriptorSet m_DescriptorSet;
			bool m_Dynamic = false;

			std::vector<api::PushConstant> m_PushConstants;
		};
	}
}
