#pragma once

#include "VKBuffer.h"
#include "Graphics/API/VertexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKVertexBuffer : public VertexBuffer, public VKBuffer
		{
		public:
			explicit VKVertexBuffer(BufferUsage usage);
			~VKVertexBuffer();

			void Resize(u32 size) override;
			void SetLayout(const Graphics::BufferLayout& layout) override;
			void SetData(u32 size, const void* data) override;
			void SetDataSub(u32 size, const void* data, u32 offset) override;

			BufferLayout GetLayout() const { return m_Layout; }

			void ReleasePointer() override;

			void Bind() override;
			void Unbind() override;
		protected:
			void* GetPointerInternal() override;

			bool m_MappedBuffer = false;

			BufferUsage m_Usage;
			u32 m_Size;
			Graphics::BufferLayout m_Layout;
		};

	}
}
