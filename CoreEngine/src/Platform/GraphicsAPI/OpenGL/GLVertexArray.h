#pragma once
#include "JM.h"
#include "Graphics/API/VertexArray.h"

namespace jm
{

	class GLVertexArray : public VertexArray
	{
	private:
		uint m_Handle;

	public:
		GLVertexArray();
		~GLVertexArray();

		inline VertexBuffer* GetBuffer(uint index = 0) override;
		void PushBuffer(VertexBuffer* buffer) override;

		void Bind() const override;
		void Unbind() const override;

		void Draw(uint count) const override;
	};
}