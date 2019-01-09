#pragma once
#include "JM.h"
#include "Graphics/API/IndexBuffer.h"
#include "GLDebug.h"

namespace jm
{

	class GLIndexBuffer : public IndexBuffer
	{
	private:
		uint m_Handle;
		uint m_Count;
		BufferUsage m_Usage;
	public:
		GLIndexBuffer(uint16* data, uint count, BufferUsage bufferUsage);
		GLIndexBuffer(uint* data  , uint count, BufferUsage bufferUsage);
		~GLIndexBuffer();

		void Bind() const override;
		void Unbind() const override;
		uint GetCount() const override;
	};
}