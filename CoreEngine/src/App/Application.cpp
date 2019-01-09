#include "JM.h"
#include "Application.h"
#include "Audio/Sound.h"
#include "Audio/SoundSystem.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/JMPhysicsEngine/JMPhysicsEngine.h"
#include "Renderer/Scene.h"
#include "Renderer/SceneManager.h"
#include "Renderer/GraphicsPipeline.h"

#include "Scripting/LuaScript.h"
#include "Graphics/API/Renderer.h"
#include "Utilities/CommonUtils.h"
#include "Engine.h"
#include "Utilities/TimeStep.h"
#include "App/Input.h"
#include "System/VFS.h"
#include "System/JobSystem.h"

namespace jm
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProperties& properties, const RenderAPI& api)
		: m_SecondTimer(0.0f), m_UpdateTimer(0), m_Frames(0), m_Updates(0)
	{
		JM_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

        system::JobSystem::Initialize();
		graphics::Context::SetRenderAPI(api);

		Engine::Instance();

        m_TimeStep = std::make_unique<TimeStep>(0.0f);
		m_Timer = std::make_unique<Timer>();

		const String root = ROOT_DIR;

		jm::VFS::Get()->Mount("CoreShaders", root + "/CoreEngine/res/shaders");
		jm::VFS::Get()->Mount("CoreMeshes", root + "/CoreEngine/res/meshes");
		jm::VFS::Get()->Mount("CoreTextures", root + "/CoreEngine/res/textures");

		m_Window = std::unique_ptr<Window>(Window::Create("Game Engine", properties));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        m_GraphicsPipeline = std::make_unique<GraphicsPipeline>();
        m_SceneManager = std::make_unique<SceneManager>();
	}

	Application::~Application()
	{
	}

	void Application::Init()
	{
		//Initialise the Window
		if (!m_Window->HasInitialised())
			Quit(true, "Window failed to initialise!");

		uint screenWidth = m_Window->GetWidth();
		uint screenHeight = m_Window->GetHeight();

		jm::Input::Create();

		graphics::Context::GetContext()->Init();

		Renderer::Init(screenWidth, screenHeight);

        system::JobSystem::Execute([] { JMPhysicsEngine::Instance(); JM_INFO("Initialised JMPhysics"); });
        system::JobSystem::Execute([] { B2PhysicsEngine::Instance(); JM_INFO("Initialised B2Physics"); });
        system::JobSystem::Execute([] { SoundSystem::Initialise();   JM_INFO("Initialised Audio"); });

        system::JobSystem::Wait();
        
        m_GraphicsPipeline->Init(screenWidth, screenHeight);

        m_CurrentState = AppState::Running;
		m_PhysicsThread = std::thread(PhysicsUpdate, 1000.0f/120.0f);
	}

	int Application::Quit(bool pause, const std::string &reason)
	{
		m_PhysicsThread.join();

        Engine::Release();
		Renderer::Release();
		JMPhysicsEngine::Release();
		B2PhysicsEngine::Release();
		Input::Release();

		Sound::DeleteSounds();
		SoundSystem::Destroy();

#ifdef JM_DEBUG_RENDERER
		DebugRenderer::Release();
#endif

		if (pause)
		{
			JM_FATAL(reason);
		}

		return 0;
	}

	void Application::DefaultControls()
	{
		const uint sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const uint sceneMax = m_SceneManager->SceneCount();

		if (Input::GetInput().GetKeyPressed(JM_KEY_1)) m_SceneManager->GetCurrentScene()->ToggleDrawObjects();
		if (Input::GetInput().GetKeyPressed(JM_KEY_2)) m_SceneManager->GetCurrentScene()->SetDrawDebugData(!m_SceneManager->GetCurrentScene()->GetDrawDebugData());
        if (Input::GetInput().GetKeyPressed(JM_KEY_P)) JMPhysicsEngine::Instance()->SetPaused(!JMPhysicsEngine::Instance()->IsPaused());

		if (Input::GetInput().GetKeyPressed(JM_KEY_J)) CommonUtils::AddSphere(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(JM_KEY_K)) CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(JM_KEY_L)) CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene());

		if (Input::GetInput().GetKeyPressed(JM_KEY_E)) m_SceneManager->JumpToScene((sceneIdx + 1) % sceneMax);
		if (Input::GetInput().GetKeyPressed(JM_KEY_Q)) m_SceneManager->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);
		if (Input::GetInput().GetKeyPressed(JM_KEY_R)) m_SceneManager->JumpToScene(sceneIdx);
		if (Input::GetInput().GetKeyPressed(JM_KEY_V)) m_Window->ToggleVSync();
	}

	void Application::PhysicsUpdate(float targetUpdateTime)
	{
		Timer t;

		float updateTimer = 0.0f;
		float updates = 0;
		float timer = 0;

		TimeStep* timeStep = new TimeStep(static_cast<float>(t.GetMS()) * 1000.0f);

        while(Application::Instance()->GetState() == AppState::Running)
		{
			float now = t.GetMS(1.0f) * 1000.0f;

			if (now - updateTimer > targetUpdateTime)
			{
				updateTimer += targetUpdateTime;

				updates++;

				timeStep->Update(now);

				JMPhysicsEngine::Instance()->Update(timeStep);
				B2PhysicsEngine::Instance()->Update(JMPhysicsEngine::Instance()->IsPaused(), timeStep);
			}

			if (t.GetMS() - timer > 1.0f)
			{
				timer += 1.0f;
				updates = 0;
			}
		}

        delete timeStep;
	}

	bool Application::OnFrame()
	{
		float now = m_Timer->GetMS(1.0f) * 1000.0f;

#ifdef JM_LIMIT_FRAMERATE
		if (now - m_UpdateTimer > Engine::Instance()->TargetFrameRate())
		{
			m_UpdateTimer += Engine::Instance()->TargetFrameRate();
#endif
			m_Updates++;
			m_Frames++;

			m_TimeStep->Update(now);

			m_GraphicsPipeline->UpdateScene(m_TimeStep.get());
			m_GraphicsPipeline->RenderScene();

			SoundSystem::Instance()->Update(m_TimeStep.get());

			m_Window->OnUpdate();

			DefaultControls();
			m_SceneManager->GetCurrentScene()->Controls();

			if (Input::GetInput().GetKeyPressed(JM_KEY_ESCAPE))
				m_CurrentState = AppState::Closing;

			Input::GetInput().ResetPressed();
#ifdef JM_LIMIT_FRAMERATE
		}
#endif

		if (m_Timer->GetMS() - m_SecondTimer > 1.0f)
		{
			m_SecondTimer += 1.0f;
            Engine::Instance()->SetFPS(m_Frames);
            Engine::Instance()->SetUPS(m_Updates);
			Engine::Instance()->SetFrametime(1000.0f/m_Frames);

			m_Frames  = 0;
			m_Updates = 0;
			m_SceneManager->GetCurrentScene()->OnTick();
		}

		return m_CurrentState == AppState::Running;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

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

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
        m_CurrentState = AppState::Closing;
		return true;
	}

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
        GetGraphicsPipeline()->OnResize(e.GetWidth(), e.GeHeight());
        return true;
    }
}
