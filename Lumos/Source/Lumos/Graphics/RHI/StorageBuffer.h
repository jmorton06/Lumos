#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class CommandBuffer;

        class StorageBuffer
        {
        public:
            virtual ~StorageBuffer() = default;
            static StorageBuffer* Create(uint32_t size, const void* data = nullptr);

            virtual void SetData(uint32_t size, const void* data)        = 0;
            virtual void Resize(uint32_t size, const void* data)         = 0;
            virtual void Unmap()                                         = 0;
            virtual void* GetBuffer() const                              = 0;
            virtual uint32_t GetSize() const                             = 0;

            template <typename T>
            T* GetPointer()
            {
                return static_cast<T*>(GetPointerInternal());
            }

        protected:
            virtual void* GetPointerInternal() = 0;

            static StorageBuffer* (*CreateFunc)(uint32_t, const void*);
        };
    }
}
