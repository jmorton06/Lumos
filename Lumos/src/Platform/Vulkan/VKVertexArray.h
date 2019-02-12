#pragma once

#include "Graphics/API/VertexArray.h"

namespace Lumos
{
	namespace graphics
	{
		class VKVertexArray : public VertexArray
		{

		public:
			VKVertexArray();
			~VKVertexArray();

			inline VertexBuffer* GetBuffer(uint index = 0) override;
			void PushBuffer(VertexBuffer* buffer) override;

			void Bind() const override;
			void Unbind() const override;

			void Draw(uint count) const override;
		};
	}
}