#include "LM.h"
#include "VertexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLVertexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKVertexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXVertexBuffer.h"
#endif
#include "Graphics/API/Context.h"

namespace Lumos
{

	VertexBuffer* VertexBuffer::Create(const BufferUsage usage)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLVertexBuffer(usage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DVertexBuffer(usage);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKVertexBuffer(usage);
#endif
		}
		return nullptr;
	}
}
