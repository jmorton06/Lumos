#pragma once

#include "JM.h"
#include "Maths/Frustum.h"

namespace jm
{
	class Scene;
	class ForwardRenderer;
	class DeferredRenderer;
	class RenderList;

    namespace graphics
    {
        namespace api
        {
			class IMGUIRenderer;
		}
	}

	struct TimeStep;

	class JM_EXPORT GraphicsPipeline
    {
	public:

		GraphicsPipeline();
		~GraphicsPipeline();

		void DebugRenderScene();
		void RenderScene();
		void UpdateScene(TimeStep* timeStep);

		bool Init(uint width, uint height);
		void InitialiseDefaults();

		inline Scene* GetScene() const { return m_pScene; }

		void Reset();
		void SetScene(Scene* scene) { m_pScene = scene; m_NewScene = true; };

		void SetScreenSize(uint width, uint height) { m_ScreenTexWidth = width; m_ScreenTexHeight = height; }
		uint GetScreenWidth()  const { return m_ScreenTexWidth; }
		uint GetScreenHeight() const { return m_ScreenTexHeight; }

		void OnResize(uint width, uint height);
		void OnIMGUI();

        maths::Frustum GetFrustum() const { return m_FrameFrustum; }

	protected:

		Scene* m_pScene;

		maths::Frustum									m_FrameFrustum;

		std::unique_ptr<ForwardRenderer>				m_ForwardRenderer;
		std::unique_ptr<DeferredRenderer>				m_DeferredRenderer;
		std::unique_ptr<RenderList>					    m_pFrameRenderList;
		std::unique_ptr<graphics::api::IMGUIRenderer>   m_IMGUIRenderer;

		bool m_NeedSceneInit = true;
		bool m_NewScene = false;

		uint	m_ScreenTexWidth, m_ScreenTexHeight;
	};
}
