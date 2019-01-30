#pragma once
#include "LM.h"
#include "Graphics/API/DescriptorSet.h"

namespace Lumos
{
    namespace graphics
    {
        class GLDescriptorSet : public api::DescriptorSet
        {
        public:
            GLDescriptorSet(api::DescriptorInfo& info) ;

            ~GLDescriptorSet() {};

            void Update(std::vector<api::ImageInfo> &imageInfos, std::vector <api::BufferInfo> &bufferInfos) override;
            void Update(std::vector <api::ImageInfo> &imageInfos) override;
            void Update(std::vector <api::BufferInfo> &bufferInfos) override;
            void SetPushConstants(std::vector<api::PushConstant>& pushConstants) override;

			void Bind(uint offset = 0);

        private:
			std::vector<api::ImageInfo> m_ImageInfos;
			std::vector <api::BufferInfo> m_BufferInfos;
            std::vector<api::PushConstant> m_PushConstants;
        };
    }
}

