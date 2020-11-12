#pragma once

namespace Lumos
{
	enum class RenderPriority
	{
		Geometry = 0,
		Lighting = 1,
		Geometry2D = 2,
		PostProcess = 3,
		Debug = 4,
		ImGui = 5,
		Screen = 6,
		Total = 7
	};

	namespace Graphics
	{
		class IRenderer;
		class Texture;
		class GBuffer;
		class TextureDepthArray;
		class GBuffer;
		class ShadowRenderer;
		class SkyboxRenderer;

		class RenderGraph
		{
		public:
			RenderGraph(u32 width, u32 height);
			~RenderGraph();

			void AddRenderer(Graphics::IRenderer* renderer);
			void SortRenderers();
			
			void Reset();
			void OnResize(u32 width, u32 height);
			
			bool GetReflectSkyBox() const { return m_ReflectSkyBox; };
			bool GetUseShadowMap() const { return m_UseShadowMap; };
			u32 GetNumShadowMaps() const { return m_NumShadowMaps; };
			TextureDepthArray* GetShadowTexture() const { return m_ShadowTexture; };
			GBuffer* GetGBuffer() const { return m_GBuffer; }
			
			void SetReflectSkyBox(bool reflect) { m_ReflectSkyBox = reflect; }
			void SetUseShadowMap(bool shadow) { m_UseShadowMap = shadow; }
			void SetNumShadowMaps(u32 num) { m_NumShadowMaps = num; }
			void SetTextureDepthArray(TextureDepthArray* texture) { m_ShadowTexture = texture; }
			
			ShadowRenderer* GetShadowRenderer() const { return m_ShadowRenderer; };
			void SetShadowRenderer(ShadowRenderer* renderer) { m_ShadowRenderer = renderer; }
			
			void SetScreenBufferSize(u32 width, u32 height) { if (width == 0) width = 1; if (height == 0) height = 1; m_ScreenBufferWidth = width; m_ScreenBufferHeight = height; }
			

		private:
			std::vector<Graphics::IRenderer*> m_Renderers;
			
			bool m_ReflectSkyBox = false;
			bool m_UseShadowMap = false;
			u32 m_NumShadowMaps = 4;
			TextureDepthArray* m_ShadowTexture = nullptr;
			Texture* m_ScreenTexture = nullptr;
			
			GBuffer* m_GBuffer = nullptr;
			
			ShadowRenderer* m_ShadowRenderer = nullptr;
			
			u32 m_ScreenBufferWidth{}, m_ScreenBufferHeight{};
		};
	}
}
