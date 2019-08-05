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
		IndexBuffer* IndexBuffer::Create(u16* data, u32 count, BufferUsage bufferUsage)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return lmnew GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return lmnew D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return lmnew Graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
			}
			return nullptr;
		}

		IndexBuffer* IndexBuffer::Create(u32* data, u32 count, BufferUsage bufferUsage)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return lmnew GLIndexBuffer(data, count, bufferUsage);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return lmnew D3DIndexBuffer(data, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return lmnew Graphics::VKIndexBuffer(data, count, bufferUsage);
#endif
			}
			return nullptr;
		}
	}
}
