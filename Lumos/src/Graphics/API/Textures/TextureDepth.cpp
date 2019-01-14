#include "JM.h"
#include "TextureDepth.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/Textures/GLTextureDepth.h"
#endif
#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKTextureDepth.h"
#endif
#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXTextureDepth.h"
#endif
#include "Graphics/API/Context.h"

namespace jm
{

	TextureDepth* TextureDepth::Create(uint width, uint height)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTextureDepth(width, height);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:		return new graphics::VKTextureDepth(width, height);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTextureDepth(width, height);
#endif
		}
		return nullptr;
	}
}