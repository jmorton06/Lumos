#include "Precompiled.h"
#include "IndexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
        IndexBuffer*(*IndexBuffer::Create16Func)(u16*, u32, BufferUsage) = nullptr;
        IndexBuffer*(*IndexBuffer::CreateFunc)(u32*, u32, BufferUsage) = nullptr;

		IndexBuffer* IndexBuffer::Create(u16* data, u32 count, BufferUsage bufferUsage)
		{
            LUMOS_ASSERT(CreateFunc, "No IndexBuffer Create Function");
            
            return Create16Func(data,count, bufferUsage);
		}

		IndexBuffer* IndexBuffer::Create(u32* data, u32 count, BufferUsage bufferUsage)
		{
            LUMOS_ASSERT(CreateFunc, "No IndexBuffer Create Function");
            
            return CreateFunc(data, count, bufferUsage);
		}
	}
}
