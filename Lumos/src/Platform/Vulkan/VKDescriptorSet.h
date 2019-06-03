#pragma once
#include "Graphics/API/DescriptorSet.h"
#include "VK.h"

namespace Lumos
{
	namespace Graphics
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

			void SetDynamicOffset(uint offset) override { m_DynamicOffset = offset; }
			uint GetDynamicOffset() const override { return m_DynamicOffset; }

		private:
			vk::DescriptorSet m_DescriptorSet;
			uint m_DynamicOffset = 0;
			Shader* m_Shader = nullptr;
			bool m_Dynamic = false;
			std::vector<PushConstant> m_PushConstants;
		};
	}
}
