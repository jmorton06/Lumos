#include "LM.h"
#include "GraphicsPipeline.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/API/IMGUIRenderer.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/Swapchain.h"
#include "Graphics/RenderList.h"
#include "Graphics/GBuffer.h"
#include "Utilities/TimeStep.h"
#include "external/imgui/imgui.h"
#include "Scene.h"
#include "App/Engine.h"
#include "App/Application.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Graphics/Renderers/ShadowRenderer.h"

namespace Lumos
{

	GraphicsPipeline::GraphicsPipeline()
		: m_pScene(nullptr)
		, m_ForwardRenderer(nullptr)
		, m_DeferredRenderer(nullptr)
		, m_IMGUIRenderer(nullptr)
	{
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
	}

	bool GraphicsPipeline::Init(uint width, uint height)
	{
		SetScreenSize(width, height);
		//m_ForwardRenderer = std::make_unique<ForwardRenderer>(m_ScreenTexWidth, m_ScreenTexHeight);
		m_DeferredRenderer = std::make_unique<DeferredRenderer>(m_ScreenTexWidth, m_ScreenTexHeight);

		m_IMGUIRenderer = std::unique_ptr<graphics::api::IMGUIRenderer>(graphics::api::IMGUIRenderer::Create(m_ScreenTexWidth, m_ScreenTexHeight, Application::Instance()->GetWindow()->GetHandle()));

		if(m_IMGUIRenderer)
			m_IMGUIRenderer->Init();

		InitialiseDefaults();

		return true;
	}

	void GraphicsPipeline::InitialiseDefaults()
	{
	}

	void GraphicsPipeline::RenderScene()
	{
		if (m_pScene == nullptr)
			return;

		m_DeferredRenderer->RenderScene(m_pScene->GetRenderList(), m_pScene);

		//for (int commandBufferIndex = 0; commandBufferIndex < m_DeferredRenderer->GetCommandBufferCount(); commandBufferIndex++)
		{
			int commandBufferIndex = Renderer::GetRenderer()->GetSwapchain()->GetCurrentBufferId();
			m_DeferredRenderer->Begin(commandBufferIndex);
			m_DeferredRenderer->Present();

			if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
			{
			}
			m_DeferredRenderer->End();
		}

		m_DeferredRenderer->PresentToScreen();
				m_IMGUIRenderer->Render(nullptr);//m_DeferredRenderer->GetCommandBuffer(commandBufferIndex));
	}

	void GraphicsPipeline::OnResize(uint width, uint height)
	{
		m_DeferredRenderer->OnResize(width, height);
	}

	void GraphicsPipeline::Reset()
	{
		InitialiseDefaults();
	}
}
