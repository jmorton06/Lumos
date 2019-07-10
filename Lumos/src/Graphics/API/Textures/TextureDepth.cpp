#include "LM.h"
#include "TextureDepth.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/Textures/GLTextureDepth.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTextureDepth.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/DirectX/DXTextureDepth.h"
#endif
#include "Graphics/API/GraphicsContext.h"

namespace Lumos
{
	namespace Graphics
	{
		TextureDepth* TextureDepth::Create(u32 width, u32 height)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureDepth(width, height);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new Graphics::VKTextureDepth(width, height);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureDepth(width, height);
#endif
			}
			return nullptr;
		}
	}
}