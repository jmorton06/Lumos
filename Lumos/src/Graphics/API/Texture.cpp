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
        Texture2D* (*Texture2D::CreateFunc)() = nullptr;
        Texture2D* (*Texture2D::CreateFromSourceFunc)(u32, u32, void*, TextureParameters, TextureLoadOptions) = nullptr;
        Texture2D* (*Texture2D::CreateFromFileFunc)(const String&, const String&, TextureParameters, TextureLoadOptions) = nullptr;
        
        TextureDepth* (*TextureDepth::CreateFunc)(u32, u32) = nullptr;
        TextureDepthArray* (*TextureDepthArray::CreateFunc)(u32, u32, u32) = nullptr;
        
        TextureCube* (*TextureCube::CreateFunc)(u32) = nullptr;
        TextureCube* (*TextureCube::CreateFromFileFunc)(const String&) = nullptr;
        TextureCube* (*TextureCube::CreateFromFilesFunc)(const String*) = nullptr;
        TextureCube* (*TextureCube::CreateFromVCrossFunc)(const String*, u32, InputFormat) = nullptr;

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
            LUMOS_CORE_ASSERT(CreateFunc, "No Texture2D Create Function");
            
            return CreateFunc();
		}

		Texture2D* Texture2D::CreateFromSource(u32 width, u32 height, void* data, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
            LUMOS_CORE_ASSERT(CreateFromSourceFunc, "No Texture2D Create Function");
            
            return CreateFromSourceFunc(width, height, data, parameters, loadOptions);
		}

		Texture2D* Texture2D::CreateFromFile(const String& name, const String& filepath, TextureParameters parameters, TextureLoadOptions loadOptions)
		{
            LUMOS_CORE_ASSERT(CreateFromFileFunc, "No Texture2D Create Function");
            
            return CreateFromFileFunc(name, filepath, parameters, loadOptions);
		}

		TextureCube* TextureCube::Create(u32 size)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No TextureCube Create Function");
            
            return CreateFunc(size);
		}

		TextureCube* TextureCube::CreateFromFile(const String& filepath)
		{
            LUMOS_CORE_ASSERT(CreateFromFileFunc, "No TextureCube Create Function");
            
            return CreateFromFileFunc(filepath);
		}

		TextureCube* TextureCube::CreateFromFiles(const String* files)
		{
            LUMOS_CORE_ASSERT(CreateFromFilesFunc, "No TextureCube Create Function");
            
            return CreateFromFilesFunc(files);
		}

		TextureCube* TextureCube::CreateFromVCross(const String* files, u32 mips, InputFormat format)
		{
            LUMOS_CORE_ASSERT(CreateFromVCrossFunc, "No TextureCube Create Function");
            
            return CreateFromVCrossFunc(files, mips, format);
		}

		TextureDepth* TextureDepth::Create(u32 width, u32 height)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No TextureDepth Create Function");
            
            return CreateFunc(width, height);
		}

		TextureDepthArray* TextureDepthArray::Create(u32 width, u32 height, u32 count)
		{
            LUMOS_CORE_ASSERT(CreateFunc, "No TextureDepthArray Create Function");
            
            return CreateFunc(width,height,count);
		}
	}
}
