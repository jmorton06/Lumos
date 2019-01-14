#pragma once
#include "LM.h"
#include "Texture.h"

namespace Lumos
{

	class LUMOS_EXPORT Texture2D : public Texture
	{
	public:
		virtual void SetData(const void* pixels) = 0;
		virtual void SetData(uint color) = 0;

		virtual uint GetWidth() const = 0;
		virtual uint GetHeight() const = 0;
	public:
		static Texture2D* Create();
		static Texture2D* Create(uint width, uint height, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		static Texture2D* CreateFromSource(uint width, uint height,void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());

		static Texture2D* CreateFromFile(const String& filepath, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		static Texture2D* CreateFromFile(const String& filepath, TextureLoadOptions loadOptions);
		static Texture2D* CreateFromFile(const String& name, const String& filepath, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		static Texture2D* CreateFromFile(const String& name, const String& filepath, TextureLoadOptions loadOptions);
		static Texture2D* Create(int width, int height, const void* pixels);

		virtual void BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow) = 0;
	};
}