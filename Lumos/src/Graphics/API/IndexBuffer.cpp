#include "LM.h"
#include "IndexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/GLIndexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXIndexBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKIndexBuffer.h"
#endif

#include "Graphics/API/Context.h"

namespace Lumos
{

	IndexBuffer* IndexBuffer::Create(uint16* data, uint count, BufferUsage bufferUsage)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
		}
		return nullptr;
	}

	IndexBuffer* IndexBuffer::Create(uint* data, uint count, BufferUsage bufferUsage)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
		}
		return nullptr;
	}
}
