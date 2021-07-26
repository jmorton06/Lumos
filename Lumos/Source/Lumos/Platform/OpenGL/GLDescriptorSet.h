#pragma once

#include "Graphics/RHI/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class GLShader;
        class GLDescriptorSet : public DescriptorSet
        {
        public:
            GLDescriptorSet(const DescriptorDesc& info);

            ~GLDescriptorSet() {};

            void Update(std::vector<Descriptor>& descriptors) override;

            void Bind(uint32_t offset = 0);

            void SetDynamicOffset(uint32_t offset) override { m_DynamicOffset = offset; }
            uint32_t GetDynamicOffset() const override { return m_DynamicOffset; }
            static void MakeDefault();

        protected:
            static DescriptorSet* CreateFuncGL(const DescriptorDesc& info);

        private:
            uint32_t m_DynamicOffset = 0;
            GLShader* m_Shader = nullptr;

            std::vector<Descriptor> m_Descriptors;
        };
    }
}
