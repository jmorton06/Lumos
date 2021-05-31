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

            void Update(std::vector<Descriptor>& descriptors) override;
            bool GetIsDynamic() const { return m_Dynamic; }

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }

            static void MakeDefault();

        protected:
            void UpdateInternal(std::vector<Descriptor>* imageInfos);

            static DescriptorSet* CreateFuncVulkan(const DescriptorInfo&);

        private:
            VkDescriptorSet m_DescriptorSet;
            uint32_t m_DynamicOffset = 0;
            Shader* m_Shader = nullptr;
            bool m_Dynamic = false;
            VkDescriptorBufferInfo* m_BufferInfoPool = nullptr;
            VkDescriptorImageInfo* m_ImageInfoPool = nullptr;
            VkWriteDescriptorSet* m_WriteDescriptorSetPool = nullptr;

            DescriptorSetInfo m_Descriptors;
        };
    }
}
