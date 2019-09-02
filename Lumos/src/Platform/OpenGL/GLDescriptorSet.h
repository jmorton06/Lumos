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

			void Bind(u32 offset = 0);

			void SetDynamicOffset(u32 offset) override { m_DynamicOffset = offset; }
			u32 GetDynamicOffset() const override { return m_DynamicOffset; }
            static void MakeDefault();
        protected:
            static DescriptorSet* CreateFuncGL(DescriptorInfo info);
        private:
			u32 m_DynamicOffset = 0;
			Shader* m_Shader = nullptr;

			std::vector<ImageInfo> m_ImageInfos;
			std::vector <BufferInfo> m_BufferInfos;
            std::vector<PushConstant> m_PushConstants;
        };
    }
}

