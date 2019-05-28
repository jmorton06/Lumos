#pragma once

#include "LM.h"

namespace lumos
{
	namespace graphics
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

			GBuffer(uint width, uint height);
			~GBuffer();

			void BuildTextures();

			void Bind(int32 mode = 0);
			void UpdateTextureSize(uint width, uint height);
			void SetReadBuffer(ScreenTextures type);

			inline uint GetWidth() const { return m_Width; }
			inline uint GetHeight() const { return m_Height; }

			inline Texture2D* GetTexture(uint index) const { return m_ScreenTex[index]; }
			inline TextureDepth* GetDepthTexture() const { return m_DepthTexture; };
			inline TextureFormat GetTextureFormat(uint index) const { return m_Formats[index]; };

		private:
			void Init();

		private:

			Texture2D* m_ScreenTex[ScreenTextures::SCREENTEX_MAX]{};
			TextureDepth* m_DepthTexture{};
			TextureFormat m_Formats[ScreenTextures::SCREENTEX_MAX];
			uint m_Width, m_Height;
		};
	}
}