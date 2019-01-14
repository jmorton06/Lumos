#pragma once
#include "LM.h"
#include "Graphics/API/Textures/Texture2D.h"

namespace Lumos
{

	class GLTexture2D : public Texture2D
	{
	private:
		String m_Name;
		String m_FileName;
		uint m_Handle;
		uint m_Width, m_Height;
		TextureParameters m_Parameters;
		TextureLoadOptions m_LoadOptions;
	public:
		GLTexture2D(uint width, uint height, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		GLTexture2D(uint width, uint height, TextureParameters parameters, TextureLoadOptions loadOptions, void* data);
		GLTexture2D(uint width, uint height, uint color, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		GLTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
		GLTexture2D(int width, int height, const void* pixels);
		GLTexture2D();
		~GLTexture2D();

		void Bind(uint slot = 0) const override;
		void Unbind(uint slot = 0) const override;

		virtual void SetData(const void* pixels) override;
		virtual void SetData(uint color) override;

		virtual uint GetHandle() const override { return m_Handle; }

		inline uint GetWidth() const override { return m_Width; }
		inline uint GetHeight() const override { return m_Height; }

		inline const String& GetName() const override { return m_Name; }
		inline const String& GetFilepath() const override { return m_FileName; }

		void BuildTexture(TextureFormat internalformat, uint width, uint height, bool depth, bool samplerShadow) override;

		byte* LoadTextureData();
		uint  LoadTexture(void* data) const;

	private:
		uint Load();
	public:
		static uint TextureFormatToGL(TextureFormat format);
		static uint TextureWrapToGL(TextureWrap wrap);
		static TextureFormat BitsToTextureFormat(uint bits);
		static uint TextureFormatToInternalFormat(uint format);
	};

}