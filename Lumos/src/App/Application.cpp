#include "lmpch.h"
#include "Application.h"

#include "Scene.h"
#include "SceneManager.h"
#include "Engine.h"
#include "Editor/Editor.h"

#include "Graphics/API/Renderer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Renderers/DebugRenderer.h"

#include "ECS/Component/MeshComponent.h"

#include "Maths/Transform.h"

#include "Utilities/CommonUtils.h"
#include "Utilities/AssetsManager.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/Profiler.h"
#include "Core/VFS.h"

#include "Scripting/LuaManager.h"

#include "ImGui/ImGuiLayer.h"

#include "Events/ApplicationEvent.h"
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

#include <imgui/imgui.h>
namespace Lumos
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProperties& properties)
		: m_UpdateTimer(0)
		, m_Frames(0)
		, m_Updates(0)
		, m_InitialProperties(properties)
	{
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

#ifdef LUMOS_EDITOR
		m_Editor = lmnew Editor(this, properties.Width, properties.Height);
#endif
		Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(properties.RenderAPI));

		Engine::Get();

		m_Timer = CreateUniqueRef<Timer>();

		const std::string root = ROOT_DIR;

		VFS::Get()->Mount("CoreShaders", root + "/Assets/shaders");
		VFS::Get()->Mount("CoreMeshes", root + "/Assets/meshes");
		VFS::Get()->Mount("CoreTextures", root + "/Assets/textures");
		VFS::Get()->Mount("CoreFonts", root + "/Assets/fonts");

		m_Window = UniqueRef<Window>(Window::Create(properties));
#ifndef LUMOS_EDITOR
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
#else
		m_Editor->BindEventFunction();
#endif

		m_SceneManager = CreateUniqueRef<SceneManager>();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
	}

	Application::~Application()
	{
		ImGui::DestroyContext();
	}

	void Application::ClearLayers()
	{
		m_LayerStack->Clear();
	}

	Scene* Application::GetCurrentScene() const
	{
		return m_SceneManager->GetCurrentScene();
	}

	void Application::Init()
	{
		// Initialise the Window
		if(!m_Window->HasInitialised())
			Quit(true, "Window failed to initialise!");

		u32 screenWidth = m_Window->GetWidth();
		u32 screenHeight = m_Window->GetHeight();

		Lumos::Input::Create();

		Graphics::GraphicsContext::GetContext()->Init();

#ifdef LUMOS_EDITOR
		m_Editor->OnInit();
#endif

		Graphics::Renderer::Init(screenWidth, screenHeight);

		// Graphics Loading on main thread
		AssetsManager::InitializeMeshes();
		m_RenderManager = CreateUniqueRef<Graphics::RenderManager>(screenWidth, screenHeight);

		m_ImGuiLayer = lmnew ImGuiLayer(false);
		m_ImGuiLayer->OnAttach();

		m_LayerStack = lmnew LayerStack();

		m_SystemManager = CreateUniqueRef<SystemManager>();

		auto audioManager = AudioManager::Create();
		if(audioManager)
		{
			audioManager->OnInit();
			m_SystemManager->RegisterSystem<AudioManager>(audioManager);
		}

		m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
		m_SystemManager->RegisterSystem<B2PhysicsEngine>();

		Material::InitDefaultTexture();

		m_CurrentState = AppState::Running;

#ifdef LUMOS_EDITOR
		DebugRenderer::Init(screenWidth, screenHeight, true);
#else
		DebugRenderer::Init(screenWidth, screenHeight, false);
#endif
	}

	int Application::Quit(bool pause, const std::string& reason)
	{
		Material::ReleaseDefaultTexture();
		Engine::Release();
		Input::Release();
		AssetsManager::ReleaseResources();
		DebugRenderer::Release();

		m_SceneManager.reset();
		m_RenderManager.reset();
		m_SystemManager.reset();

		delete m_LayerStack;
		delete m_ImGuiLayer;

#ifdef LUMOS_EDITOR
		lmdel m_Editor;
#endif

		Graphics::Renderer::Release();
		Graphics::GraphicsContext::Release();

		if(pause)
		{
			LUMOS_LOG_ERROR("{0}", reason);
		}

		return 0;
	}

	void Application::SetActiveCamera(Camera* camera)
	{
		m_ActiveCamera = camera;
	}

	Maths::Vector2 Application::GetWindowSize() const
	{
#ifdef LUMOS_EDITOR
		return Maths::Vector2(m_SceneViewWidth, m_SceneViewHeight);
#else
		return Maths::Vector2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
#endif
	}

	bool Application::OnFrame()
	{
		float now = m_Timer->GetMS(1.0f);

#ifdef LUMOS_LIMIT_FRAMERATE
		if(now - m_UpdateTimer > Engine::Get().TargetFrameRate())
		{
			m_UpdateTimer += Engine::Get().TargetFrameRate();
#endif

			Profiler::Get().Update(now);

			auto& ts = Engine::GetTimeStep();
			ts.Update(now);

			ImGuiIO& io = ImGui::GetIO();
			io.DeltaTime = ts.GetMillis();

			Application& app = Application::Get();
			app.GetWindow()->UpdateCursorImGui();

			ImGui::NewFrame();

			{
				LUMOS_PROFILE_BLOCK("Application::Update");
				OnUpdate(ts);
				m_Updates++;
			}

			if(!m_Minimized)
			{
				LUMOS_PROFILE_BLOCK("Application::Render");
				OnRender();
				m_Frames++;
			}

			Input::GetInput()->ResetPressed();
			m_Window->OnUpdate();

			if(Input::GetInput()->GetKeyPressed(LUMOS_KEY_ESCAPE))
				m_CurrentState = AppState::Closing;
#ifdef LUMOS_LIMIT_FRAMERATE
		}
#endif

		if(m_Timer->GetMS() - m_SecondTimer > 1.0f)
		{
			LUMOS_PROFILE_BLOCK("Application::FrameRateCalc");

			m_SecondTimer += 1.0f;
			Engine::Get().SetFPS(m_Frames);
			Engine::Get().SetUPS(m_Updates);
			Engine::Get().SetFrametime(1000.0f / m_Frames);

			m_Frames = 0;
			m_Updates = 0;
			m_SceneManager->GetCurrentScene()->OnTick();
		}

		if(m_EditorState == EditorState::Next)
			m_EditorState = EditorState::Paused;

		m_SceneManager->ApplySceneSwitch();

		return m_CurrentState != AppState::Closing;
	}

	void Application::OnRender()
	{
		if(m_LayerStack->GetCount() > 0 || m_SceneManager->GetCurrentScene()->GetLayers()->GetCount() > 0)
		{
			//#ifdef LUMOS_EDITOR
			//			if(m_SceneViewSizeUpdated)
			//			{
			//				if(m_SceneViewWidth > 0 && m_SceneViewHeight > 0)
			//				{
			//					OnSceneViewSizeUpdated(m_SceneViewWidth, m_SceneViewHeight);
			//				}
			//
			//				m_SceneViewSizeUpdated = false;
			//			}
			//#endif
			Graphics::Renderer::GetRenderer()->Begin();
			DebugRenderer::Reset();

			m_SystemManager->OnDebugDraw();
#ifdef LUMOS_EDITOR
			m_Editor->DebugDraw();
			m_Editor->OnRender();
#endif

			m_LayerStack->OnRender(m_SceneManager->GetCurrentScene());
			m_SceneManager->GetCurrentScene()->GetLayers()->OnRender(m_SceneManager->GetCurrentScene());
			DebugRenderer::Render(m_SceneManager->GetCurrentScene());
			m_ImGuiLayer->OnRender(m_SceneManager->GetCurrentScene());

			Graphics::Renderer::GetRenderer()->Present();
		}
	}

	void Application::OnUpdate(const TimeStep& dt)
	{
		const u32 sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const u32 sceneMax = m_SceneManager->SceneCount();

		if(Input::GetInput()->GetKeyPressed(InputCode::Key::P))
			Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(!Application::Get().GetSystem<LumosPhysicsEngine>()->IsPaused());
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::P))
			Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(!Application::Get().GetSystem<B2PhysicsEngine>()->IsPaused());

		Camera* cameraComponent = nullptr;

		auto cameraView = m_SceneManager->GetCurrentScene()->GetRegistry().view<Camera>();
		if(cameraView.size() > 0)
		{
			cameraComponent = m_SceneManager->GetCurrentScene()->GetRegistry().try_get<Camera>(cameraView[0]);
		}

		if(cameraComponent)
		{
			if(Input::GetInput()->GetKeyPressed(InputCode::Key::J))
				CommonUtils::AddSphere(m_SceneManager->GetCurrentScene(), cameraComponent->GetPosition(), -cameraComponent->GetForwardDirection());
			if(Input::GetInput()->GetKeyPressed(InputCode::Key::K))
				CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene(), cameraComponent->GetPosition(), -cameraComponent->GetForwardDirection());
			if(Input::GetInput()->GetKeyPressed(InputCode::Key::L))
				CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene(), cameraComponent->GetPosition(), -cameraComponent->GetForwardDirection());
		}

		if(Input::GetInput()->GetKeyPressed(InputCode::Key::G))
			m_SceneManager->SwitchScene((sceneIdx + 1) % sceneMax);
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::H))
			m_SceneManager->SwitchScene(sceneIdx);
		if(Input::GetInput()->GetKeyPressed(InputCode::Key::V))
			m_Window->ToggleVSync();

#ifdef LUMOS_EDITOR
		m_Editor->OnUpdate(dt);

		if(Application::Get().GetEditorState() != EditorState::Paused
			&& Application::Get().GetEditorState() != EditorState::Preview)
#endif
		{
			m_SceneManager->GetCurrentScene()->OnUpdate(dt);
			m_SystemManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
			LuaManager::Get().OnUpdate(m_SceneManager->GetCurrentScene());
		}

		if(!m_Minimized)
		{
			m_LayerStack->OnUpdate(dt, m_SceneManager->GetCurrentScene());
			m_SceneManager->GetCurrentScene()->GetLayers()->OnUpdate(dt, m_SceneManager->GetCurrentScene());
			m_ImGuiLayer->OnUpdate(dt, m_SceneManager->GetCurrentScene());
		}
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		m_ImGuiLayer->OnEvent(e);
		if(e.Handled())
			return;
		m_LayerStack->OnEvent(e);

		if(e.Handled())
			return;

		m_SceneManager->GetCurrentScene()->OnEvent(e);

		Input::GetInput()->OnEvent(e);
	}

	void Application::Run()
	{
		m_UpdateTimer = m_Timer->GetMS(1.0f);
		while(OnFrame())
		{
		}

		Quit();
	}

	void Application::OnNewScene(Scene* scene)
	{
#ifdef LUMOS_EDITOR
		m_SceneViewSizeUpdated = true;
		m_Editor->OnNewScene(scene);
#endif
	}

	void Application::OnExitScene()
	{
	}

	void Application::PushLayerInternal(Layer* layer, bool overlay, bool sceneAdded)
	{
		if(overlay)
			m_LayerStack->PushOverlay(layer);
		else
			m_LayerStack->PushLayer(layer);

		layer->OnAttach();
	}

	void Application::PushLayer(Layer* layer)
	{
		PushLayerInternal(layer, false, true);
	}

	void Application::PushOverLay(Layer* overlay)
	{
		PushLayerInternal(overlay, true, true);
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_CurrentState = AppState::Closing;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		Graphics::GraphicsContext::GetContext()->WaitIdle();

		int width = e.GetWidth(), height = e.GetHeight();

		if(width == 0 || height == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		m_RenderManager->OnResize(width, height);
		Graphics::Renderer::GetRenderer()->OnResize(width, height);
		DebugRenderer::OnResize(width, height);

		Graphics::GraphicsContext::GetContext()->WaitIdle();

		return false;
	}

	void Application::OnImGui()
	{
#ifdef LUMOS_EDITOR
		if(m_AppType == AppType::Editor)
			m_Editor->OnImGui();
#endif
		Application::Get().GetSceneManager()->GetCurrentScene()->OnImGui();
	}

	void Application::OnSceneViewSizeUpdated(u32 width, u32 height)
	{
		Graphics::GraphicsContext::GetContext()->WaitIdle();

		WindowResizeEvent e(width, height);
		if(width == 0 || height == 0)
		{
			m_Minimized = true;
		}
		m_Minimized = false;
		m_RenderManager->OnResize(width, height);
		m_LayerStack->OnEvent(e);
		m_SceneManager->GetCurrentScene()->GetLayers()->OnEvent(e);
		DebugRenderer::OnResize(width, height);

		Graphics::GraphicsContext::GetContext()->WaitIdle();
	}
}
