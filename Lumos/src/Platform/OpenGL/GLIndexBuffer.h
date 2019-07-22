#pragma once
#include "LM.h"
#include "Graphics/API/IndexBuffer.h"
#include "GLDebug.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLIndexBuffer : public IndexBuffer
		{
		private:
			u32 m_Handle;
			u32 m_Count;
			BufferUsage m_Usage;
		public:
			GLIndexBuffer(uint16* data, u32 count, BufferUsage bufferUsage);
			GLIndexBuffer(u32* data, u32 count, BufferUsage bufferUsage);
			~GLIndexBuffer();

			void Bind(CommandBuffer* commandBuffer) const override;
			void Unbind() const override;
			u32 GetCount() const override;
			void SetCount(u32 m_index_count) override { m_Count = m_index_count; };

		};
	}
}