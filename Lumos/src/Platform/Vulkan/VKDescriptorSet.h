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

			vk::DescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

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
			vk::DescriptorSet m_DescriptorSet;
			u32 m_DynamicOffset = 0;
			Shader* m_Shader = nullptr;
			bool m_Dynamic = false;
			std::vector<PushConstant> m_PushConstants;
			vk::DescriptorBufferInfo* m_BufferInfoPool = nullptr;
			vk::DescriptorImageInfo* m_ImageInfoPool = nullptr;
		};
	}
}
