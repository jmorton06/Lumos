#pragma once
#include "Graphics/API/DescriptorSet.h"
#include "Dependencies/vulkan/vulkan.h"

namespace jm
{
	namespace graphics
	{
		class VKDescriptorSet : public api::DescriptorSet
		{
		public:
			VKDescriptorSet(api::DescriptorInfo info);
			~VKDescriptorSet();

			VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

			void Update(std::vector<api::ImageInfo>& imageInfos, std::vector<api::BufferInfo>& bufferInfos) override;
			void Update(std::vector<api::BufferInfo>& bufferInfos) override;
			void Update(std::vector<api::ImageInfo>& imageInfos) override;

			bool GetIsDynamic() const { return m_Dynamic; }

		private:
			VkDescriptorSet m_DescriptorSet;
			bool m_Dynamic = false;
		};
	}
}
