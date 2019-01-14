#pragma once
#include "JM.h"
#include "Graphics/API/DescriptorSet.h"

namespace jm
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

			void Bind(uint offset = 0);

        private:
			std::vector<api::ImageInfo> m_ImageInfos;
			std::vector <api::BufferInfo> m_BufferInfos;
        };
    }
}

