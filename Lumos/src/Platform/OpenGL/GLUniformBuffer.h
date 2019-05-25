#pragma once
#include "LM.h"
#include "Graphics/API/UniformBuffer.h"

namespace lumos
{
    namespace graphics
    {
		class GLShader;
        
		class GLUniformBuffer : public UniformBuffer
        {
        public:
            GLUniformBuffer();
            ~GLUniformBuffer();

			void Init(uint32_t size, const void* data) override;
			void SetData(uint32_t size, const void* data) override;
			void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) override;

			void Bind(uint slot, GLShader* shader, String& name);

			byte* GetBuffer() const override { return m_Data; };

            uint32_t GetSize()      const { return m_Size; }
            uint32_t GetTypeSize()  const { return m_DynamicTypeSize; }
            bool GetDynamic()       const { return m_Dynamic; }

        private:
			byte* m_Data = nullptr;
            uint32_t m_Size = 0;
            uint32_t m_DynamicTypeSize = 0;
            bool m_Dynamic = false;
			uint m_Handle;
			uint m_Index;
        };
    }
}

