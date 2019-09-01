#include "LM.h"
#include "VertexArray.h"
#include "VertexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLVertexArray.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXVertexArray.h"
#endif
#include "Graphics/API/GraphicsContext.h"
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKVertexArray.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
        VertexArray*(*VertexArray::CreateFunc)() = nullptr;

		VertexArray* VertexArray::Create()
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No VertexArray Create Function");
            
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
