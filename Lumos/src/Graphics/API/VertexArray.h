#pragma once

#include "LM.h"
#include "VertexBuffer.h"

namespace Lumos
{

	class LUMOS_EXPORT VertexArray
	{
	public:
		virtual ~VertexArray() = default;
		virtual VertexBuffer* GetBuffer(uint index = 0) = 0;
		virtual void PushBuffer(VertexBuffer* buffer) = 0;

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		void DeleteBuffers();
		uint GetCount() const { return static_cast<uint>(m_Buffers.size()); }

		virtual void Draw(uint count) const = 0;
		static VertexArray* Create();

	protected:

		std::vector<VertexBuffer*> m_Buffers;
	};
}
