#include "LM.h"
#include "IndexBuffer.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLIndexBuffer.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXIndexBuffer.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKIndexBuffer.h"
#endif

#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
		IndexBuffer* IndexBuffer::Create(uint16* data, uint count, BufferUsage bufferUsage)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new Graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
			}
			return nullptr;
		}

		IndexBuffer* IndexBuffer::Create(uint* data, uint count, BufferUsage bufferUsage)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new Graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
			}
			return nullptr;
		}
	}
}
