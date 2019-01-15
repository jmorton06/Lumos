#include "LM.h"
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
#include "Graphics/ImGuiLayer.h"

#include <imgui/imgui.h>

namespace Lumos
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProperties& properties, const RenderAPI& api)
		: m_SecondTimer(0.0f), m_UpdateTimer(0), m_Frames(0), m_Updates(0)
	{
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

        system::JobSystem::Initialize();
		graphics::Context::SetRenderAPI(api);

		Engine::Instance();

        m_TimeStep = std::make_unique<TimeStep>(0.0f);
		m_Timer = std::make_unique<Timer>();

		const String root = ROOT_DIR;

		Lumos::VFS::Get()->Mount("CoreShaders", root + "/Lumos/res/shaders");
		Lumos::VFS::Get()->Mount("CoreMeshes", root + "/Lumos/res/meshes");
		Lumos::VFS::Get()->Mount("CoreTextures", root + "/Lumos/res/textures");

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

		Lumos::Input::Create();

		graphics::Context::GetContext()->Init();

		Renderer::Init(screenWidth, screenHeight);

        system::JobSystem::Execute([] { JMPhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised JMPhysics"); });
        system::JobSystem::Execute([] { B2PhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised B2Physics"); });
        system::JobSystem::Execute([] { SoundSystem::Initialise();   LUMOS_CORE_INFO("Initialised Audio"); });

        system::JobSystem::Wait();

		PushOverLay(new ImGuiLayer());
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

#ifdef LUMOS_DEBUG_RENDERER
		DebugRenderer::Release();
#endif

		if (pause)
		{
            LUMOS_CORE_ERROR("{0}", reason);
		}

		return 0;
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

#ifdef LUMOS_LIMIT_FRAMERATE
		if (now - m_UpdateTimer > Engine::Instance()->TargetFrameRate())
		{
			m_UpdateTimer += Engine::Instance()->TargetFrameRate();
#endif
			m_Updates++;
			m_Frames++;

			m_TimeStep->Update(now);

			for (Layer* layer : m_LayerStack)
				layer->OnUpdate(m_TimeStep.get());

			m_SceneManager->GetCurrentScene()->OnUpdate(m_TimeStep.get());

			SoundSystem::Instance()->Update(m_TimeStep.get());

			OnRender();
            m_Window->OnUpdate();
            OnUpdate(m_TimeStep.get());

			if (Input::GetInput().GetKeyPressed(LUMOS_KEY_ESCAPE))
				m_CurrentState = AppState::Closing;

			Input::GetInput().ResetPressed();
#ifdef LUMOS_LIMIT_FRAMERATE
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

	void Application::OnRender()
	{
		m_GraphicsPipeline->RenderScene();

		for (Layer* layer : m_LayerStack)
			layer->OnRender(m_SceneManager->GetCurrentScene());
	}

	void Application::OnUpdate(TimeStep* dt)
	{
		const uint sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const uint sceneMax = m_SceneManager->SceneCount();

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_1)) m_SceneManager->GetCurrentScene()->ToggleDrawObjects();
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_2)) m_SceneManager->GetCurrentScene()->SetDrawDebugData(!m_SceneManager->GetCurrentScene()->GetDrawDebugData());
        if (Input::GetInput().GetKeyPressed(LUMOS_KEY_P)) JMPhysicsEngine::Instance()->SetPaused(!JMPhysicsEngine::Instance()->IsPaused());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_J)) CommonUtils::AddSphere(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_K)) CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_L)) CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_E)) m_SceneManager->JumpToScene((sceneIdx + 1) % sceneMax);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_Q)) m_SceneManager->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_R)) m_SceneManager->JumpToScene(sceneIdx);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_V)) m_Window->ToggleVSync();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

		Input::GetInput().OnEvent(e);

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(e);
			if (e.Handled())
				break;
		}
	}

	void Application::Run()
	{
		m_UpdateTimer = m_Timer->GetMS(1.0f);
		while (OnFrame())
		{

		}

		Quit();
	}

	void Application::PushLayer(Layer * layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverLay(Layer * overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
        m_CurrentState = AppState::Closing;
		return true;
	}

    bool Application::OnWindowResize(WindowResizeEvent &e)
    {
		Renderer::GetRenderer()->OnResize(e.GetWidth(), e.GetHeight());
        GetGraphicsPipeline()->OnResize(e.GetWidth(), e.GetHeight());
        return false;
    }

	void Application::OnImGui()
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

		m_SceneManager->GetCurrentScene()->OnIMGUI();
	}
}
