#include "LM.h"
#include "Texture.h"

#include "Utilities/LoadImage.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/GLTexture.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/directx/DXTexture.h"
#endif

#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture.h"
#endif

#include "Graphics/API/GraphicsContext.h"


namespace Lumos
{
	namespace Graphics
	{
		u8 Texture::GetStrideFromFormat(const TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:				return 3;
			case TextureFormat::RGBA:				return 4;
			case TextureFormat::R8:					return 1;
			case TextureFormat::RG8:				return 2;
			case TextureFormat::RGB8:				return 3;
			case TextureFormat::RGBA8:				return 4;
			default: return 0;
			}
		}

		Texture2D* Texture2D::Create()
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTexture2D();
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTexture2D();
#endif
			}
			return nullptr;
		}

		Texture2D* Texture2D::CreateFromSource(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTexture2D(width, height, data, parameters, loadOptions);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTexture2D(width, height, data, parameters, loadOptions);
#endif
			}
			return nullptr;
		}

		Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:	return new GLTexture2D(name, filepath, parameters, loadOptions);
#endif 
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:	return new VKTexture2D(name, filepath, parameters, loadOptions);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::Create(u32 size)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(size);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(size);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTextureCube(size);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromFile(const String& filepath)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(filepath, filepath);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(filepath, filepath);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTextureCube(filepath, filepath);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromFiles(const String* files)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(files[0], files);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(files[0], files);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTextureCube(files[0], files);
#endif
			}
			return nullptr;
		}

		TextureCube* TextureCube::CreateFromVCross(const String* files, u32 mips)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTextureCube(files[0], files, mips, InputFormat::VERTICAL_CROSS);
#endif
			}
			return nullptr;
		}

		TextureDepth* TextureDepth::Create(u32 width, u32 height)
		{
			switch (Graphics::GraphicsContext::GetRenderAPI())
			{
#ifdef LUMOS_RENDER_API_OPENGL
			case RenderAPI::OPENGL:		return new GLTextureDepth(width, height);
#endif
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN:		return new VKTextureDepth(width, height);
#endif
#ifdef LUMOS_RENDER_API_DIRECT3D
			case RenderAPI::DIRECT3D:	return new D3DTextureDepth(width, height);
#endif
			}
			return nullptr;
		}

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
			case RenderAPI::VULKAN:     return new VKTextureDepthArray(width, height, count);
#endif
			}
			return nullptr;
		}
	}
}