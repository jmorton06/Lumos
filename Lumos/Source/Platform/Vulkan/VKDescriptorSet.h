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
			VKDescriptorSet(const DescriptorInfo& info);
			~VKDescriptorSet();

			VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

			void Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos) override;
			void Update(std::vector<BufferInfo>& bufferInfos) override;
			void Update(std::vector<ImageInfo>& imageInfos) override;
            
			void SetPushConstants(std::vector<PushConstant>& pushConstants) override;
			bool GetIsDynamic() const { return m_Dynamic; }

			const std::vector<PushConstant>& GetPushConstants() const { return m_PushConstants; }

			void SetDynamicOffset(u32 offset) override { m_DynamicOffset = offset; }
			u32 GetDynamicOffset() const override { return m_DynamicOffset; }
            
			static void MakeDefault();
		protected:
            
            void UpdateInternal(std::vector<ImageInfo>* imageInfos, std::vector<BufferInfo>* bufferInfos);
            
			static DescriptorSet* CreateFuncVulkan(const DescriptorInfo&);
		private:
			VkDescriptorSet m_DescriptorSet;
			u32 m_DynamicOffset = 0;
			Shader* m_Shader = nullptr;
			bool m_Dynamic = false;
			std::vector<PushConstant> m_PushConstants;
			VkDescriptorBufferInfo* m_BufferInfoPool = nullptr;
			VkDescriptorImageInfo* m_ImageInfoPool = nullptr;
			VkWriteDescriptorSet* m_WriteDescriptorSetPool = nullptr;
		};
	}
}
