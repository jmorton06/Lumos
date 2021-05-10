#pragma once
#include "Graphics/API/UniformBuffer.h"

namespace Lumos
{
    namespace Graphics
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

            void Bind(uint32_t slot, GLShader* shader, std::string& name);

            uint8_t* GetBuffer() const override
            {
                return m_Data;
            };

            uint32_t GetSize() const
            {
                return m_Size;
            }
            uint32_t GetTypeSize() const
            {
                return m_DynamicTypeSize;
            }
            bool GetDynamic() const
            {
                return m_Dynamic;
            }
            uint32_t GetHandle() const
            {
                return m_Handle;
            }

            static void MakeDefault();

        protected:
            static UniformBuffer* CreateFuncGL();
            static UniformBuffer* CreateDataFuncGL(uint32_t, const void*);

        private:
            uint8_t* m_Data = nullptr;
            uint32_t m_Size = 0;
            uint32_t m_DynamicTypeSize = 0;
            bool m_Dynamic = false;
            uint32_t m_Handle;
            uint32_t m_Index;
        };
    }
}
