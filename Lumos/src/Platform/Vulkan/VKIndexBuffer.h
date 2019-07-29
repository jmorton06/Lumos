#pragma once

#include "VKBuffer.h"
#include "Graphics/API/IndexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKIndexBuffer : public IndexBuffer, public VKBuffer
		{
		public:
			VKIndexBuffer(u16* data, u32 count, BufferUsage bufferUsage);
			VKIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage);
			~VKIndexBuffer();

			void Bind(CommandBuffer* commandBuffer) const override;
			void Unbind() const override;
			u32 GetCount() const override;
			u32 GetSize() const override;
			void SetCount(u32 m_index_count) override { m_Count = m_index_count; };

		private:
			BufferUsage m_Usage;
			u32 m_Count;
			u32 m_Size;
			Graphics::BufferLayout m_Layout;
		};
	}
}
