#include "JM.h"
#include "Texture2D.h"

#ifdef JM_RENDER_API_OPENGL
#include "Platform/GraphicsAPI/OpenGL/Textures/GLTexture2D.h"
#endif

#ifdef JM_RENDER_API_DIRECT3D
#include "graphics/directx/DXTexture2D.h"
#endif

#include "Graphics/API/Context.h"
#ifdef JM_RENDER_API_VULKAN
#include "Platform/GraphicsAPI/Vulkan/VKTexture2D.h"
#endif

namespace jm
{

	Texture2D* Texture2D::Create()
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D();
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D();
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D();
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::Create(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(width, height, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(width, height, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromSource(uint width, uint height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(width, height, parameters, loadOptions, data);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(width, height, TextureParameters(), TextureLoadOptions());
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, parameters, loadOptions, data);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(filepath, filepath, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(filepath, filepath, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(filepath, filepath, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& filepath, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(filepath, filepath, TextureParameters(), loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(name, filepath, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(name, filepath, parameters, loadOptions);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(name, filepath, parameters, loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureLoadOptions loadOptions)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:		return new GLTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:	return new D3DTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
#ifdef JM_RENDER_API_VULKAN
		case RenderAPI::VULKAN:	return new graphics::VKTexture2D(name, filepath, TextureParameters(), loadOptions);
#endif
		}
		return nullptr;
	}

	Texture2D* Texture2D::Create(int width, int height, const void* pixels)
	{
		switch (graphics::Context::GetRenderAPI())
		{
#ifdef JM_RENDER_API_OPENGL
		case RenderAPI::OPENGL:	return new GLTexture2D(width, height, pixels);
#endif
#ifdef JM_RENDER_API_DIRECT3D
		case RenderAPI::DIRECT3D:  return new D3DTexture2D(width, height);// , pixels);
#endif
#ifdef JM_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new graphics::VKTexture2D(width, height, pixels);
#endif
		}
		return nullptr;
	}
}
