#include "LM.h"
#include "Texture2D.h"
#include "Utilities/LoadImage.h"

#ifdef LUMOS_RENDER_API_OPENGL
#include "Platform/OpenGL/Textures/GLTexture2D.h"
#endif

#ifdef LUMOS_RENDER_API_DIRECT3D
#include "Graphics/directx/DXTexture2D.h"
#endif

#include "Graphics/API/GraphicsContext.h"
#ifdef LUMOS_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture2D.h"
#endif

namespace Lumos
{
	namespace Graphics
	{
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

		Texture2D* Texture2D::CreateFromSource(uint width, uint height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
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
			case RenderAPI::OPENGL	:	return new GLTexture2D(name, filepath, parameters, loadOptions);
#endif 
#ifdef LUMOS_RENDER_API_VULKAN
			case RenderAPI::VULKAN	:	return new VKTexture2D(name, filepath, parameters, loadOptions);
#endif
			}
			return nullptr;
		}
	}
}
