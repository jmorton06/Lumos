#pragma once
#include "LM.h"
#include "Graphics/API/BufferLayout.h"

namespace Lumos
{
	namespace Graphics
	{
		enum class BufferUsage
		{
			STATIC, DYNAMIC, STREAM
		};

		class LUMOS_EXPORT VertexBuffer
		{
		public:
			virtual ~VertexBuffer() = default;
			virtual void Resize(u32 size) = 0;
			virtual void SetLayout(const Graphics::BufferLayout& layout) = 0;
			virtual void SetData(u32 size, const void* data) = 0;
			virtual void SetDataSub(u32 size, const void* data, u32 offset) = 0;

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
}
