#pragma once
#include "LM.h"
#include "Graphics/API/Textures/Texture2D.h"

namespace Lumos
{
	namespace Graphics
	{
		class GLTexture2D : public Texture2D
		{
		private:
			String m_Name;
			String m_FileName;
			u32 m_Handle;
			u32 m_Width, m_Height;
			TextureParameters m_Parameters;
			TextureLoadOptions m_LoadOptions;
		public:
			GLTexture2D(u32 width, u32 height, void* data, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			GLTexture2D(const String& name, const String& filename, TextureParameters parameters = TextureParameters(), TextureLoadOptions loadOptions = TextureLoadOptions());
			GLTexture2D();
			~GLTexture2D();

			void Bind(u32 slot = 0) const override;
			void Unbind(u32 slot = 0) const override;

			virtual void SetData(const void* pixels) override;

			virtual void* GetHandle() const override { return (void*)(size_t)m_Handle; }

			inline u32 GetWidth() const override { return m_Width; }
			inline u32 GetHeight() const override { return m_Height; }

			inline const String& GetName() const override { return m_Name; }
			inline const String& GetFilepath() const override { return m_FileName; }

			void BuildTexture(TextureFormat internalformat, u32 width, u32 height, bool depth, bool samplerShadow) override;

			u8* LoadTextureData();
			u32  LoadTexture(void* data) const;

		private:
			u32 Load(void* data);
		};
	}
}