#pragma once

#include "LM.h"

namespace Lumos
{
	namespace Graphics
	{
		class VertexBuffer;
		class CommandBuffer;

		class LUMOS_EXPORT VertexArray
		{
		public:
			virtual ~VertexArray() = default;
			virtual VertexBuffer* GetBuffer(u32 index = 0) = 0;
			virtual void PushBuffer(VertexBuffer* buffer) = 0;

			virtual void Bind(CommandBuffer* commandBuffer = nullptr) const = 0;
			virtual void Unbind() const = 0;

			void DeleteBuffers();
			u32 GetCount() const { return static_cast<u32>(m_Buffers.size()); }

			static VertexArray* Create();

        protected:
            static VertexArray* (*CreateFunc)();

			std::vector<VertexBuffer*> m_Buffers;
		};
	}
}
