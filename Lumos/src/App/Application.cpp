#include "LM.h"
#include "Application.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/RenderManager.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "App/Scene.h"
#include "App/SceneManager.h"
#include "Utilities/AssetsManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/API/Renderer.h"
#include "Graphics/API/Context.h"
#include "Utilities/CommonUtils.h"
#include "Engine.h"
#include "Utilities/TimeStep.h"
#include "App/Input.h"
#include "System/VFS.h"
#include "System/JobSystem.h"
#include "Audio/AudioManager.h"
#include "Scene.h"

#include <imgui/imgui.h>
#include "Graphics/Layers/ImGuiLayer.h"
#include "Editor.h"
#include "System/Profiler.h"

namespace Lumos
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProperties& properties, const RenderAPI& api)
		: m_UpdateTimer(0), m_Frames(0), m_Updates(0)
	{
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		LUMOS_PROFILE(system::Profiler::OnBegin());
		LUMOS_PROFILE(system::Profiler::OnBeginRange("StartUp", false, "", true));
	

#ifdef  LUMOS_EDITOR
		m_Editor = new Editor(this, properties.Width, properties.Height);
#endif
		graphics::Context::SetRenderAPI(api);

#ifdef LUMOS_EDITOR
#ifdef LUMOS_RENDER_API_OPENGL
		if (api == RenderAPI::OPENGL)
			m_Editor->m_FlipImGuiImage = true;
#endif
#endif

		Engine::Instance();

		m_TimeStep = std::make_unique<TimeStep>(0.0f);
		m_Timer = std::make_unique<Timer>();

		const String root = ROOT_DIR;

		Lumos::VFS::Get()->Mount("CoreShaders", root + "/Lumos/res/shaders");
		Lumos::VFS::Get()->Mount("CoreMeshes", root + "/Lumos/res/meshes");
		Lumos::VFS::Get()->Mount("CoreTextures", root + "/Lumos/res/textures");

		m_Window = std::unique_ptr<Window>(Window::Create("Game Engine", properties));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		m_SceneManager = std::make_unique<SceneManager>();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
	}

	Application::~Application()
	{
#ifdef  LUMOS_EDITOR
		delete m_Editor;
#endif
		ImGui::DestroyContext();
	}

	void Application::ClearLayers()
	{
		m_LayerStack->Clear();
	}

	void Application::Init()
	{
		//Initialise the Window
		if (!m_Window->HasInitialised())
			Quit(true, "Window failed to initialise!");

		uint screenWidth = m_Window->GetWidth();
		uint screenHeight = m_Window->GetHeight();

		Lumos::Input::Create();

		graphics::Context::GetContext()->Init();

#ifdef  LUMOS_EDITOR
		m_Editor->OnInit();
#endif

		Renderer::Init(screenWidth, screenHeight);

		system::JobSystem::Execute([] { LumosPhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised LumosPhysics"); });
		system::JobSystem::Execute([] { B2PhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised B2Physics"); });

        system::JobSystem::Execute([&]
        {
            m_AudioManager = std::unique_ptr<AudioManager>(AudioManager::Create());
            m_AudioManager->OnInit();
        });
        
        //Graphics Loading on main thread
        AssetsManager::InitializeMeshes();
        m_RenderManager = std::make_unique<RenderManager>(screenWidth, screenHeight);

        system::JobSystem::Wait();
        
        m_LayerStack = new LayerStack();
        PushOverLay(new ImGuiLayer(false));
        
		m_Systems.emplace_back(m_AudioManager.get());
		m_Systems.emplace_back(LumosPhysicsEngine::Instance());
		m_Systems.emplace_back(B2PhysicsEngine::Instance());
        
		m_CurrentState = AppState::Running;

		LUMOS_PROFILE(system::Profiler::OnEndRange("StartUp", false, "", true));
		LUMOS_PROFILE(system::Profiler::OnEnd());
	}

	int Application::Quit(bool pause, const std::string &reason)
	{
		Engine::Release();
		LumosPhysicsEngine::Release();
		B2PhysicsEngine::Release();
		Input::Release();
		AssetsManager::ReleaseMeshes();

		m_LayerStack->Clear();
		m_SceneManager.release();

		Renderer::Release();

		if (pause)
		{
			LUMOS_CORE_ERROR("{0}", reason);
		}

		return 0;
	}

	maths::Vector2 Application::GetWindowSize() const
	{
		return maths::Vector2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
	}

	bool Application::OnFrame()
	{
		float now = m_Timer->GetMS(1.0f) * 1000.0f;

		LUMOS_PROFILE(system::Profiler::OnBegin());
		LUMOS_PROFILE(system::Profiler::OnBeginRange("MainLoop"));

#ifdef LUMOS_LIMIT_FRAMERATE
		if (now - m_UpdateTimer > Engine::Instance()->TargetFrameRate())
		{
			m_UpdateTimer += Engine::Instance()->TargetFrameRate();
#endif

			m_TimeStep->Update(now);

			{
				OnUpdate(m_TimeStep.get());
				m_Updates++;
			}

			{
				OnRender();
				m_Frames++;
			}

			Input::GetInput().ResetPressed();
			m_Window->OnUpdate();

			if (Input::GetInput().GetKeyPressed(LUMOS_KEY_ESCAPE))
				m_CurrentState = AppState::Closing;
#ifdef LUMOS_LIMIT_FRAMERATE
		}
#endif

		if (m_Timer->GetMS() - m_SecondTimer > 1.0f)
		{
			m_SecondTimer += 1.0f;
			Engine::Instance()->SetFPS(m_Frames);
			Engine::Instance()->SetUPS(m_Updates);
			Engine::Instance()->SetFrametime(1000.0f / m_Frames);

			m_Frames = 0;
			m_Updates = 0;
			m_SceneManager->GetCurrentScene()->OnTick();
		}

		if (m_EditorState == EditorState::Next)
			m_EditorState = EditorState::Paused;

		LUMOS_PROFILE(system::Profiler::OnEndRange("MainLoop"));
		LUMOS_PROFILE(system::Profiler::OnEnd());
		return m_CurrentState != AppState::Closing;
	}

	void Application::OnRender()
	{
		LUMOS_PROFILE(system::Profiler::OnBeginRange("Render", true, "MainLoop"));
		if (m_LayerStack->GetCount() > 0)
		{
			LUMOS_PROFILE(system::Profiler::OnBeginRange("Begin", true, "Render"));
			Renderer::GetRenderer()->Begin();
			LUMOS_PROFILE(system::Profiler::OnEndRange("Begin", true, "Render"));

			m_LayerStack->OnRender(m_SceneManager->GetCurrentScene());

			LUMOS_PROFILE(system::Profiler::OnBeginRange("Present", true, "Render"));
			Renderer::GetRenderer()->Present();
			LUMOS_PROFILE(system::Profiler::OnEndRange("Present", true, "Render"));
		}

		LUMOS_PROFILE(system::Profiler::OnEndRange("Render", true, "MainLoop"));
	}

	void Application::OnUpdate(TimeStep* dt)
	{
		LUMOS_PROFILE(system::Profiler::OnBeginRange("Update", true, "MainLoop"));
		const uint sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const uint sceneMax = m_SceneManager->SceneCount();

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_1)) m_SceneManager->GetCurrentScene()->ToggleDrawObjects();
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_2)) m_SceneManager->GetCurrentScene()->SetDrawDebugData(!m_SceneManager->GetCurrentScene()->GetDrawDebugData());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_P)) LumosPhysicsEngine::Instance()->SetPaused(!LumosPhysicsEngine::Instance()->IsPaused());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_P)) B2PhysicsEngine::Instance()->SetPaused(!B2PhysicsEngine::Instance()->IsPaused());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_J)) CommonUtils::AddSphere(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_K)) CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_L)) CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_E)) m_SceneManager->JumpToScene((sceneIdx + 1) % sceneMax);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_Q)) m_SceneManager->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_R)) m_SceneManager->JumpToScene(sceneIdx);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_V)) m_Window->ToggleVSync();

#ifdef LUMOS_EDITOR
		if (Application::Instance()->GetEditorState() != EditorState::Paused && Application::Instance()->GetEditorState() != EditorState::Preview)
#endif
		{
			m_SceneManager->GetCurrentScene()->OnUpdate(m_TimeStep.get());
			
			for (auto& system : m_Systems)
            {
				LUMOS_PROFILE(system::Profiler::OnBeginRange(system->GetName() , true, "Update"));
                system->OnUpdate(m_TimeStep.get());
				LUMOS_PROFILE(system::Profiler::OnEndRange(system->GetName(), true, "Update"));
            }
		}

		m_LayerStack->OnUpdate(m_TimeStep.get(), m_SceneManager->GetCurrentScene());

		LUMOS_PROFILE(system::Profiler::OnEndRange("Update", true, "MainLoop"));
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		m_LayerStack->OnEvent(e);

		if (e.Handled())
			return;

		m_SceneManager->GetCurrentScene()->OnEvent(e);

		Input::GetInput().OnEvent(e);
	}

	void Application::Run()
	{
		m_UpdateTimer = m_Timer->GetMS(1.0f);
		while (OnFrame())
		{

		}

		Quit();
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack->PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverLay(Layer* overlay)
	{
		m_LayerStack->PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnNewScene(Scene * scene)
	{
#ifdef LUMOS_EDITOR
		m_Editor->OnNewScene(scene);
#endif
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_CurrentState = AppState::Closing;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent &e)
	{
        auto windowSize = GetWindowSize();
		m_RenderManager->OnResize(e.GetWidth(), e.GetHeight());
		Renderer::GetRenderer()->OnResize(e.GetWidth(), e.GetHeight());
		return false;
	}

	void Application::OnImGui()
	{
#ifdef LUMOS_EDITOR
		if(m_AppType == AppType::Editor)
			m_Editor->OnImGui();
#endif
    }
}
