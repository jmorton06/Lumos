#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class UniformBuffer
        {
        public:
            virtual ~UniformBuffer() = default;
            static UniformBuffer* Create();
            static UniformBuffer* Create(uint32_t size, const void* data);

            virtual void Init(uint32_t size, const void* data) = 0;
            virtual void SetData(uint32_t size, const void* data) = 0;
            virtual void SetDynamicData(uint32_t size, uint32_t typeSize, const void* data) = 0;

            virtual uint8_t* GetBuffer() const = 0;

        protected:
            static UniformBuffer* (*CreateFunc)();
            static UniformBuffer* (*CreateDataFunc)(uint32_t, const void*);
        };
    }
}
