#include "lmpch.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

namespace Lumos
{
	namespace Graphics
	{
        VertexArray*(*VertexArray::CreateFunc)() = nullptr;

		VertexArray* VertexArray::Create()
		{
            LUMOS_ASSERT(CreateFunc, "No VertexArray Create Function");
            
            return CreateFunc();
		}

		void VertexArray::DeleteBuffers()
		{
			for (auto buffer : m_Buffers)
			{
				delete buffer;
				buffer = nullptr;
			}

			m_Buffers.clear();
		}
	}
}
