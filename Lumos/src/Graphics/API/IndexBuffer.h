#pragma once

#include "LM.h"

#include "Graphics/API/VertexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT IndexBuffer
		{
		public:
			virtual ~IndexBuffer() = default;
			virtual void Bind() const = 0;
			virtual void Unbind() const = 0;

			virtual uint GetCount() const = 0;
			virtual uint GetSize() const { return 0; }
			virtual void SetCount(uint m_index_count) = 0;
		public:
			static IndexBuffer* Create(uint16* data, uint count, BufferUsage bufferUsage = BufferUsage::STATIC);
			static IndexBuffer* Create(uint* data, uint count, BufferUsage bufferUsage = BufferUsage::STATIC);
		};
	}
}
