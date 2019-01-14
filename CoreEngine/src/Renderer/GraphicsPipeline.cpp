#include "JM.h"
#include "GraphicsPipeline.h"
#include "Utilities/AssetsManager.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/RenderList.h"
#include "Graphics/GBuffer.h"
#include "Utilities/TimeStep.h"
#include "external/imgui/imgui.h"
#include "Scene.h"
#include "App/Engine.h"
#include "App/Application.h"

namespace jm
{

	GraphicsPipeline::GraphicsPipeline()
		: m_pScene(nullptr)
		, m_ForwardRenderer(nullptr)
		, m_DeferredRenderer(nullptr)
		, m_IMGUIRenderer(nullptr)
	{
		m_pFrameRenderList = std::make_unique<RenderList>();

		if (!RenderList::AllocateNewRenderList(m_pFrameRenderList.get(), true))
		{
			JM_CORE_ERROR("Unable to allocate scene render list! - Try using less shadow maps", "");
		}
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		AssetsManager::ReleaseMeshes();
	}

	bool GraphicsPipeline::Init(uint width, uint height)
	{
		SetScreenSize(width, height);
		//m_ForwardRenderer = std::make_unique<ForwardRenderer>(m_ScreenTexWidth, m_ScreenTexHeight);
		m_DeferredRenderer = std::make_unique<DeferredRenderer>(m_ScreenTexWidth, m_ScreenTexHeight);

		m_IMGUIRenderer = std::unique_ptr<graphics::api::IMGUIRenderer>(graphics::api::IMGUIRenderer::Create(m_ScreenTexWidth, m_ScreenTexHeight, Application::Instance()->GetWindow()->GetHandle()));

		if(m_IMGUIRenderer)
			m_IMGUIRenderer->Init();

		AssetsManager::InitializeMeshes();

		InitialiseDefaults();

		return true;
	}

	void GraphicsPipeline::InitialiseDefaults()
	{
		DebugRenderer::ClearLog();
		m_NeedSceneInit = true;
	}

	void GraphicsPipeline::RenderScene()
	{
		if (m_pScene == nullptr)
			return;

		if(m_NeedSceneInit)
		{
			//m_ForwardRenderer->InitScene(m_pScene);
			m_DeferredRenderer->InitScene(m_pScene, m_NewScene);

			m_NewScene = false;
			m_NeedSceneInit = false;
		}

		m_pScene->GetCamera()->BuildViewMatrix();
		m_FrameFrustum.FromMatrix(m_pScene->GetCamera()->GetProjectionMatrix() * m_pScene->GetCamera()->GetViewMatrix());

		m_pScene->BuildWorldMatrices();

		m_pFrameRenderList->UpdateCameraWorldPos(m_pScene->GetCamera()->GetPosition());
		m_pFrameRenderList->RemoveExcessObjects(m_FrameFrustum);
		m_pFrameRenderList->SortLists();
		m_pScene->InsertToRenderList(m_pFrameRenderList.get(), m_FrameFrustum);

		if (m_pScene->GetDrawObjects())
		{
			m_DeferredRenderer->RenderScene(m_pFrameRenderList.get(), m_pScene);

			for (int commandBufferIndex = 0; commandBufferIndex < m_DeferredRenderer->GetCommandBufferCount(); commandBufferIndex++)
			{
				m_DeferredRenderer->Begin(commandBufferIndex);
				m_DeferredRenderer->Present();

				if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
				{
					m_IMGUIRenderer->Render(m_DeferredRenderer->GetCommandBuffer(commandBufferIndex));
				}
				m_DeferredRenderer->End();
			}

			m_DeferredRenderer->PresentToScreen();

		}
	}

	void GraphicsPipeline::DebugRenderScene()
	{
	}

	void GraphicsPipeline::OnResize(uint width, uint height)
	{
		m_NeedSceneInit = true;

		Renderer::GetRenderer()->OnResize(width, height);

		m_DeferredRenderer->OnResize(width, height);

		if (m_pScene)
		{
			if (m_pScene->GetCamera())
				m_pScene->GetCamera()->UpdateProjectionMatrix(static_cast<float>(m_ScreenTexWidth), static_cast<float>(m_ScreenTexHeight));
		}
	}

	void GraphicsPipeline::OnIMGUI()
	{
		ImGui::Begin("Engine Information");
		ImGui::Text("--------------------------------");
		ImGui::Text("Physics Engine: %s (Press P to toggle)", JMPhysicsEngine::Instance()->IsPaused() ? "Paused" : "Enabled");
		ImGui::Text("Number Of Collision Pairs  : %5.2i", JMPhysicsEngine::Instance()->GetNumberCollisionPairs());
		ImGui::Text("Number Of Physics Objects  : %5.2i", JMPhysicsEngine::Instance()->GetNumberPhysicsObjects());
		ImGui::Text("--------------------------------");
		ImGui::Text("FPS : %5.2i", Engine::Instance()->GetFPS());
		ImGui::Text("UPS : %5.2i", Engine::Instance()->GetUPS());
		ImGui::Text("Frame Time : %5.2f ms", Engine::Instance()->GetFrametime());
		ImGui::Text("--------------------------------");
		ImGui::End();
	}

	void GraphicsPipeline::Reset()
	{
		m_pFrameRenderList->RemoveAllObjects();
		InitialiseDefaults();
	}
}
