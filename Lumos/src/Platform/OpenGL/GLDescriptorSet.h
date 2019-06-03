#pragma once
#include "LM.h"
#include "Graphics/API/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLDescriptorSet : public DescriptorSet
        {
        public:
            GLDescriptorSet(DescriptorInfo& info) ;

            ~GLDescriptorSet() {};

            void Update(std::vector<ImageInfo> &imageInfos, std::vector <BufferInfo> &bufferInfos) override;
            void Update(std::vector <ImageInfo> &imageInfos) override;
            void Update(std::vector <BufferInfo> &bufferInfos) override;
            void SetPushConstants(std::vector<PushConstant>& pushConstants) override;

			void Bind(uint offset = 0);

			void SetDynamicOffset(uint offset) override { m_DynamicOffset = offset; }
			uint GetDynamicOffset() const override { return m_DynamicOffset; }

        private:
			uint m_DynamicOffset = 0;
			Shader* m_Shader = nullptr;

			std::vector<ImageInfo> m_ImageInfos;
			std::vector <BufferInfo> m_BufferInfos;
            std::vector<PushConstant> m_PushConstants;
        };
    }
}

