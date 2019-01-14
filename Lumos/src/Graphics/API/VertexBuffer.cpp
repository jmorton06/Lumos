#include "JM.h"
#include "VertexBuffer.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLVertexBuffer.h"
#endif
#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKVertexBuffer.h"
#endif
#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXVertexBuffer.h"
#endif
#include "Graphics/API/Context.h"

namespace jm
{

	VertexBuffer* VertexBuffer::Create(const BufferUsage usage)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLVertexBuffer(usage);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DVertexBuffer(usage);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKVertexBuffer(usage);
#endif
		}
		return nullptr;
	}
}
