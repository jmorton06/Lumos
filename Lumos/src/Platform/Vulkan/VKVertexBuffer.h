#pragma once

#include "VKBuffer.h"
#include "Graphics/API/VertexBuffer.h"

namespace Lumos
{
	namespace graphics
	{
		class VKVertexBuffer : public VertexBuffer, public VKBuffer
		{
		public:
			explicit VKVertexBuffer(BufferUsage usage);
			~VKVertexBuffer();

			void Resize(uint size) override;
			void SetLayout(const graphics::BufferLayout& layout) override;
			void SetData(uint size, const void* data) override;
			void SetDataSub(uint size, const void* data, uint offset) override;

			BufferLayout GetLayout() const { return m_Layout; }

			void ReleasePointer() override;

			void Bind() override;
			void Unbind() override;
		protected:
			void* GetPointerInternal() override;

			BufferUsage m_Usage;
			uint m_Size;
			graphics::BufferLayout m_Layout;
		};

	}
}
