#include "LM.h"
#include "Texture2D.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/Textures/GLTexture2D.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "graphics/directx/DXTexture2D.h"
#endif

#include "Graphics/API/Context.h"
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture2D.h"
#endif

namespace Lumos
{

	Texture2D* Texture2D::Create()
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D();
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D();
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::Create(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(width, height, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(width, height, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromSource(uint width, uint height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(width, height, parameters, loadOptions, data);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(width, height, TextureParameters(), TextureLoadOptions());
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, parameters, loadOptions, data);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(filepath, filepath, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(filepath, filepath, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(filepath, filepath, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& filepath, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(name, filepath, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(name, filepath, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(name, filepath, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::Create(int width, int height, void* pixels)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef LUMOS_RENDER_API_OPENGL
		case RenderAPI::OPENGL:	return new GLTexture2D(width, height, pixels);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:  return new D3DTexture2D(width, height);// , pixels);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, pixels);
#endif
		}
		return nullptr;
	}
}
