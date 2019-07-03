#include "LM.h"
#include "TextureDepthArray.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/Textures/GLTextureDepthArray.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXTextureDepthArray.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTextureDepthArray.h"
#endif
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
		TextureDepthArray* TextureDepthArray::Create(u32 width, u32 height, u32 count)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureDepthArray(width, height, count);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureDepthArray(width, height, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:     return new Graphics::VKTextureDepthArray(width, height, count);
#endif
			}
			return nullptr;
		}
	}
}