#pragma once

#include "Graphics/API/VertexArray.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKVertexArray : public VertexArray
		{

		public:
			VKVertexArray();
			~VKVertexArray();

			inline VertexBuffer* GetBuffer(u32 index = 0) override;
			void PushBuffer(VertexBuffer* buffer) override;

			void Bind() const override;
			void Unbind() const override;

			void Draw(u32 count) const override;
		};
	}
}