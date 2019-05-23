#include "LM.h"
#include "TextureCube.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/Textures/GLTextureCube.h"
#endif
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTextureCube.h"
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/directx/DXTextureCube.h"
#endif

#include "Graphics/API/GraphicsContext.h"

namespace lumos
{
	namespace graphics
	{
		TextureCube* TextureCube::Create(uint size)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(size);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(size);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTextureCube(size);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromFile(const String& filepath)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(filepath, filepath);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(filepath, filepath);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTextureCube(filepath, filepath);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromFiles(const String* files)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(files[0], files);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(files[0], files);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTextureCube(files[0], files);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromVCross(const String* files, int32 mips)
		{
			switch (graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
			}
			return nullptr;
		}
	}
}
