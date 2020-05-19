#pragma once
#include "lmpch.h"

namespace Lumos
{
	namespace Graphics
	{
		class Texture;
		class TextureCube;
		class TextureDepthArray;
		class GBuffer;
		class ShadowRenderer;
		class SkyboxRenderer;

		class LUMOS_EXPORT RenderManager
		{
		public:
			RenderManager(u32 width, u32 height);
			~RenderManager();

			RenderManager(RenderManager const&) = delete;
			RenderManager& operator=(RenderManager const&) = delete;

			void Reset();
			void OnResize(u32 width, u32 height);

			bool GetReflectSkyBox() const { return m_ReflectSkyBox; };
			bool GetUseShadowMap() const { return m_UseShadowMap; };
			u32 GetNumShadowMaps() const { return m_NumShadowMaps; };
			TextureCube* GetSkyBox() const { return m_SkyBoxTexture; };
			TextureDepthArray* GetShadowTexture() const { return m_ShadowTexture; };
			GBuffer* GetGBuffer() const { return m_GBuffer; }

			void SetReflectSkyBox(bool reflect) { m_ReflectSkyBox = reflect; }
			void SetUseShadowMap(bool shadow) { m_UseShadowMap = shadow; }
			void SetNumShadowMaps(u32 num) { m_NumShadowMaps = num; }
			void SetSkyBoxTexture(TextureCube* cube) { m_SkyBoxTexture = cube; }
			void SetTextureDepthArray(TextureDepthArray* texture) { m_ShadowTexture = texture; }

			ShadowRenderer* GetShadowRenderer() const { return m_ShadowRenderer; };
			void SetShadowRenderer(ShadowRenderer* renderer) { m_ShadowRenderer = renderer; }

			void SetScreenBufferSize(u32 width, u32 height) { if (width == 0) width = 1; if (height == 0) height = 1; m_ScreenBufferWidth = width; m_ScreenBufferHeight = height; }
			//TODO refactor

		private:
			bool m_ReflectSkyBox = false;
			bool m_UseShadowMap = false;
			u32 m_NumShadowMaps = 4;
			TextureCube* m_SkyBoxTexture = nullptr;
			TextureDepthArray* m_ShadowTexture = nullptr;
			Texture* m_ScreenTexture = nullptr;

			GBuffer* m_GBuffer = nullptr;

			ShadowRenderer* m_ShadowRenderer = nullptr;

			u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};
		};
	}
}

