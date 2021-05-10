#pragma once

#include "Graphics/API/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLDescriptorSet : public DescriptorSet
        {
        public:
            GLDescriptorSet(const DescriptorInfo& info);

            ~GLDescriptorSet() {};

            void Update(std::vector<ImageInfo>& imageInfos, std::vector<BufferInfo>& bufferInfos) override;
            void Update(std::vector<ImageInfo>& imageInfos) override;
            void Update(std::vector<BufferInfo>& bufferInfos) override;

            void Bind(uint32_t offset = 0);

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
            static void MakeDefault();

        protected:
            static DescriptorSet* CreateFuncGL(const DescriptorInfo& info);

        private:
            uint32_t m_DynamicOffset = 0;
            Shader* m_Shader = nullptr;

            std::vector<ImageInfo> m_ImageInfos;
            std::vector<BufferInfo> m_BufferInfos;
        };
    }
}
