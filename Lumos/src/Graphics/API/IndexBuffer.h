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

			virtual u32 GetCount() const = 0;
			virtual u32 GetSize() const { return 0; }
			virtual void SetCount(u32 m_index_count) = 0;
		public:
			static IndexBuffer* Create(uint16* data, u32 count, BufferUsage bufferUsage = BufferUsage::STATIC);
			static IndexBuffer* Create(u32* data, u32 count, BufferUsage bufferUsage = BufferUsage::STATIC);
		};
	}
}
