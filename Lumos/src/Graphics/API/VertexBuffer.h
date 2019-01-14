#pragma once

#include "LM.h"
#include "Graphics/API/BufferLayout.h"

namespace Lumos
{

	enum class BufferUsage
	{
		STATIC, DYNAMIC, STREAM
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;
		virtual void Resize(uint size) = 0;
		virtual void SetLayout(const graphics::BufferLayout& layout) = 0;
		virtual void SetData(uint size, const void* data) = 0;
		virtual void SetDataSub(uint size, const void* data, uint offset) = 0;

		virtual void ReleasePointer() = 0;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		template<typename T>
		T* GetPointer()
		{
			return static_cast<T*>(GetPointerInternal());
		}
	protected:
		virtual void* GetPointerInternal() = 0;
	public:
		static VertexBuffer* Create(BufferUsage usage = BufferUsage::STATIC);
	};
}
