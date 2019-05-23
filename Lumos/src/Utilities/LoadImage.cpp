#include "LM.h"
#include "LoadImage.h"

#include "System/VFS.h"

#ifdef FREEIMAGE
#include <FreeImage.h>
#include <Utilities.h>
#else
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif


namespace lumos
{
	byte* LoadImageFromFile(const char* filename, uint* width, uint* height, uint* bits, bool flipY)
	{
        String filePath = String(filename);
#ifdef LUMOS_PLATFORM_MOBILE
        if (filePath.find_last_of("/") != String::npos)
           filePath = filePath.substr(filePath.find_last_of("/") + 1);
#endif

		std::string physicalPath;
		if (!VFS::Get()->ResolvePhysicalPath(filePath, physicalPath))
			return nullptr;

		filename = physicalPath.c_str();

#ifdef FREEIMAGE
		FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
		FIBITMAP* dib = nullptr;
		fif = FreeImage_GetFileType(filename, 0);
		if (fif == FIF_UNKNOWN)
			fif = FreeImage_GetFIFFromFilename(filename);
		if (fif == FIF_UNKNOWN)
			return nullptr;

		if (FreeImage_FIFSupportsReading(fif))
			dib = FreeImage_Load(fif, filename);

		LUMOS_CORE_ASSERT(dib, "Could not load image '{0}'!", filename);

		FIBITMAP* bitmap = FreeImage_ConvertTo32Bits(dib);
		FreeImage_Unload(dib);

		byte* pixels = FreeImage_GetBits(bitmap);
		uint w = FreeImage_GetWidth(bitmap);
		uint h = FreeImage_GetHeight(bitmap);
		uint b = FreeImage_GetBPP(bitmap);

		if (flipY)
			FreeImage_FlipVertical(bitmap);

		if (FreeImage_GetRedMask(bitmap) == 0xff0000)
			SwapRedBlue32(bitmap);

		if (width)
			*width = w;
		if (height)
			*height = h;
		if (bits)
			*bits = b;

		int32 size = w * h * (b / 8);
		byte* result = new byte[size];
		memcpy(result, pixels, size);
		FreeImage_Unload(bitmap);
#else
		int texWidth = 0, texHeight = 0, texChannels = 0;
		stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		LUMOS_CORE_ASSERT(pixels, "Could not load image '{0}'!", filename);

		if (width)
			*width = texWidth;
		if (height)
			*height = texHeight;
		if (bits)
			*bits = 32;// texChannels * 8;// texChannels;	  //32 bits for 4 bytes r g b a 

		const int32 size = texWidth * texHeight * 4;// (b / 8);
		byte* result = new byte[size];
		memcpy(result, pixels, size);

		stbi_image_free(pixels);
#endif
		return result;
	}

	byte* LoadImageFromFile(const String& filename, uint* width, uint* height, uint* bits, bool flipY)
	{
		return LoadImageFromFile(filename.c_str(), width, height, bits, flipY);
	}
}
