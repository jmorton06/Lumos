#pragma once

#include "VKBuffer.h"
#include "Graphics/API/IndexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKIndexBuffer : public IndexBuffer, public VKBuffer
		{
			BufferUsage m_Usage;
			uint m_Count;
			uint m_Size;
			Graphics::BufferLayout m_Layout;
		public:
			VKIndexBuffer(uint16* data, uint count, BufferUsage bufferUsage);
			VKIndexBuffer(uint* data, uint count, BufferUsage bufferUsage);
			~VKIndexBuffer();

			void Bind() const override;
			void Unbind() const override;
			uint GetCount() const override;
			uint GetSize() const override;
			void SetCount(uint m_index_count) override { m_Count = m_index_count; };
		};
	}
}
