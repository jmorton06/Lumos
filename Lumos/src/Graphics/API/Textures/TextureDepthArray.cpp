#include "LM.h"
#include "TextureDepthArray.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/Textures/GLTextureDepthArray.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/DirectX/DXTextureDepthArray.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKTextureDepthArray.h"
#endif
#include "Graphics/API/Context.h"

namespace Lumos
{

	TextureDepthArray* TextureDepthArray::Create(uint width, uint height, uint count)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTextureDepthArray(width, height, count);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTextureDepthArray(width, height, count);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:     return new graphics::VKTextureDepthArray(width,height,count);
#endif
		}
		return nullptr;
	}
}