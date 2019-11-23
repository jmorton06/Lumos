#pragma once

#include "lmpch.h"

namespace Lumos
{
	namespace Graphics
	{
		class Framebuffer;
		class Texture2D;
		class TextureDepth;
		enum class TextureFormat;

		enum LUMOS_EXPORT ScreenTextures
		{
			SCREENTEX_DEPTH = 0,	//Depth Buffer
			SCREENTEX_STENCIL = 0,	//Stencil Buffer (Same Tex as Depth)
			SCREENTEX_COLOUR = 1,	//Main Render
			SCREENTEX_POSITION = 2,	//Deferred Render - World Space Positions
			SCREENTEX_NORMALS = 3,	//Deferred Render - World Space Normals
			SCREENTEX_PBR = 4,	//Metallic/Roughness/Ao Stored Here
			SCREENTEX_OFFSCREEN0 = 5,	//Extra Textures for multipass post processing
			SCREENTEX_OFFSCREEN1 = 6,    //Or Displaying scene in editor mode
			SCREENTEX_MAX
		};

		class LUMOS_EXPORT GBuffer
		{
		public:

			GBuffer(u32 width, u32 height);
			~GBuffer();

			void BuildTextures();

			void Bind(i32 mode = 0);
			void UpdateTextureSize(u32 width, u32 height);
			void SetReadBuffer(ScreenTextures type);

			_FORCE_INLINE_ u32 GetWidth() const { return m_Width; }
			_FORCE_INLINE_ u32 GetHeight() const { return m_Height; }

			_FORCE_INLINE_ Texture2D* GetTexture(u32 index) const { return m_ScreenTex[index]; }
			_FORCE_INLINE_ TextureDepth* GetDepthTexture() const { return m_DepthTexture; };
			_FORCE_INLINE_ TextureFormat GetTextureFormat(u32 index) const { return m_Formats[index]; };

		private:
			void Init();

		private:

			Texture2D* m_ScreenTex[ScreenTextures::SCREENTEX_MAX]{};
			TextureDepth* m_DepthTexture{};
			TextureFormat m_Formats[ScreenTextures::SCREENTEX_MAX];
			u32 m_Width, m_Height;
		};
	}
}
