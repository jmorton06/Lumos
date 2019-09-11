#include "lmpch.h"
#include "VertexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLVertexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKVertexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXVertexBuffer.h"
#endif
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
        VertexBuffer*(*VertexBuffer::CreateFunc)(const BufferUsage&) = nullptr;

		VertexBuffer* VertexBuffer::Create(const BufferUsage& usage)
		{
            LUMOS_ASSERT(CreateFunc, "No VertexBuffer Create Function");
            
            return CreateFunc(usage);
		}
	}
}
