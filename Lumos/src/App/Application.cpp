#include "LM.h"
#include "Application.h"
#include "Graphics/API/Textures/Texture2D.h"
#include "Graphics/API/Textures/TextureCube.h"
#include "Graphics/API/Textures/TextureDepthArray.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/RenderManager.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "App/Scene.h"
#include "App/SceneManager.h"
#include "Utilities/AssetsManager.h"
#include "Scripting/LuaScript.h"
#include "Graphics/API/Renderer.h"
#include "Utilities/CommonUtils.h"
#include "Engine.h"
#include "Utilities/TimeStep.h"
#include "App/Input.h"
#include "System/VFS.h"
#include "System/JobSystem.h"
#include "Audio/AudioManager.h"
#include "Graphics/GBuffer.h"

#include <imgui/imgui.h>
#include <imgui/plugins/ImGuizmo.h>
#include "Graphics/Layers/ImGuiLayer.h"

namespace Lumos
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const WindowProperties& properties, const RenderAPI& api)
		: m_UpdateTimer(0), m_Frames(0), m_Updates(0)
	{
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		system::JobSystem::Initialize();
		graphics::Context::SetRenderAPI(api);

#ifdef LUMOS_RENDER_API_OPENGL
		if (api == RenderAPI::OPENGL)
			m_FlipImGuiImage = true;
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

#ifdef  LUMOS_EDITOR
		m_SceneViewSize = maths::Vector2(static_cast<float>(properties.Width), static_cast<float>(properties.Height));
#endif
	}

	Application::~Application()
	{
		ImGui::DestroyContext();
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

		system::JobSystem::Execute([] { LumosPhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised JMPhysics"); });
		system::JobSystem::Execute([] { B2PhysicsEngine::Instance(); LUMOS_CORE_INFO("Initialised B2Physics"); });
		system::JobSystem::Wait();

		AssetsManager::InitializeMeshes();

		m_LayerStack = new LayerStack();
		m_RenderManager = std::make_unique<RenderManager>(screenWidth, screenHeight);

		PushOverLay(new ImGuiLayer(false));

		m_AudioManager = std::unique_ptr<AudioManager>(AudioManager::Create());
		m_AudioManager->OnInit();

		m_CurrentState = AppState::Running;
		m_PhysicsThread = std::thread(PhysicsUpdate, 1000.0f / 120.0f);
	}

	int Application::Quit(bool pause, const std::string &reason)
	{
		m_PhysicsThread.join();

		Engine::Release();
		LumosPhysicsEngine::Release();
		B2PhysicsEngine::Release();
		Input::Release();
		AssetsManager::ReleaseMeshes();

		m_LayerStack->Clear();
		m_SceneManager.release();

		Renderer::Release();

		//SoundSystem::Destroy();

		if (pause)
		{
			LUMOS_CORE_ERROR("{0}", reason);
		}

		return 0;
	}

	maths::Vector2 Application::GetWindowSize() const
	{
#ifdef LUMOS_EDITOR
		return m_SceneViewSize;
#else
		return m_Window->GetScreenSize();
#endif
	}

	void Application::PhysicsUpdate(float targetUpdateTime)
	{
		Timer t;

		float updateTimer = 0.0f;

		TimeStep* timeStep = new TimeStep(static_cast<float>(t.GetMS()) * 1000.0f);

		while (Application::Instance()->GetState() == AppState::Running)
		{
			float now = t.GetMS(1.0f) * 1000.0f;

			if (now - updateTimer > targetUpdateTime)
			{
				updateTimer += targetUpdateTime;

				timeStep->Update(now);

				LumosPhysicsEngine::Instance()->Update(timeStep);
				B2PhysicsEngine::Instance()->Update(LumosPhysicsEngine::Instance()->IsPaused(), timeStep);
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

		return m_CurrentState == AppState::Running;
	}

	void Application::OnRender()
	{
		if (m_LayerStack->GetCount() > 0)
		{
			Renderer::GetRenderer()->Begin();

			m_LayerStack->OnRender(m_SceneManager->GetCurrentScene());

			Renderer::GetRenderer()->Present();
		}
	}

	void Application::OnUpdate(TimeStep* dt)
	{
		const uint sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const uint sceneMax = m_SceneManager->SceneCount();

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_1)) m_SceneManager->GetCurrentScene()->ToggleDrawObjects();
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_2)) m_SceneManager->GetCurrentScene()->SetDrawDebugData(!m_SceneManager->GetCurrentScene()->GetDrawDebugData());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_P)) LumosPhysicsEngine::Instance()->SetPaused(!LumosPhysicsEngine::Instance()->IsPaused());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_J)) CommonUtils::AddSphere(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_K)) CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_L)) CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene());

		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_E)) m_SceneManager->JumpToScene((sceneIdx + 1) % sceneMax);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_Q)) m_SceneManager->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_R)) m_SceneManager->JumpToScene(sceneIdx);
		if (Input::GetInput().GetKeyPressed(LUMOS_KEY_V)) m_Window->ToggleVSync();

		m_SceneManager->GetCurrentScene()->OnUpdate(m_TimeStep.get());

		m_AudioManager->OnUpdate();

		m_LayerStack->OnUpdate(m_TimeStep.get());
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

	void Application::OnGuizmo()
	{
		auto& io = ImGui::GetIO();

		ImGuiWindowFlags windowFlags = 0;
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin("Scene", nullptr, windowFlags);
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

		m_SceneViewSize = maths::Vector2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

		m_SceneManager->GetCurrentScene()->GetCamera()->SetAspectRatio(static_cast<float>(ImGui::GetWindowSize().x) / static_cast<float>(ImGui::GetWindowSize().y));

		ImGui::Image(m_RenderManager->GetGBuffer()->m_ScreenTex[SCREENTEX_OFFSCREEN0]->GetHandle(), io.DisplaySize, ImVec2(0.0f, m_FlipImGuiImage ? 1.0f : 0.0f), ImVec2(1.0f, m_FlipImGuiImage ? 0.0f : 1.0f));
		
		if (m_Selected)
			m_Selected->OnGuizmo();

		ImGui::End();
	}

	void Application::OnImGui()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit")) { Application::Instance()->SetAppState(AppState::Closing); }
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("X")) { Application::Instance()->SetAppState(AppState::Closing); }
			ImGui::EndMainMenuBar();
		}

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		auto pos = viewport->Pos;
		pos.y += 19.0f; //Main Menu size
		ImGui::SetNextWindowPos(pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		auto windowFlags
			= ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_NoMove
			| ImGuiDockNodeFlags_PassthruDockspace;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f));

		ImGuiWindowFlags window_flags = 0;
		ImGui::Begin("Engine", NULL, window_flags);
		if (ImGui::RadioButton("Translate", m_ImGuizmoOperation == ImGuizmo::TRANSLATE))
			m_ImGuizmoOperation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", m_ImGuizmoOperation == ImGuizmo::ROTATE))
			m_ImGuizmoOperation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", m_ImGuizmoOperation == ImGuizmo::SCALE))
			m_ImGuizmoOperation = ImGuizmo::SCALE;

		ImGui::NewLine();
		ImGui::Text("Physics Engine: %s (Press P to toggle)", LumosPhysicsEngine::Instance()->IsPaused() ? "Paused" : "Enabled");
		ImGui::Text("Number Of Collision Pairs  : %5.2i", LumosPhysicsEngine::Instance()->GetNumberCollisionPairs());
		ImGui::Text("Number Of Physics Objects  : %5.2i", LumosPhysicsEngine::Instance()->GetNumberPhysicsObjects());
		ImGui::NewLine();
		ImGui::Text("FPS : %5.2i", Engine::Instance()->GetFPS());
		ImGui::Text("UPS : %5.2i", Engine::Instance()->GetUPS());
		ImGui::Text("Frame Time : %5.2f ms", Engine::Instance()->GetFrametime());
		ImGui::NewLine();
        ImGui::Text("Scene : %s", m_SceneManager->GetCurrentScene()->GetSceneName().c_str());

		if (ImGui::TreeNode("Colour Texture"))
		{
			ImGui::Image(m_RenderManager->GetGBuffer()->m_ScreenTex[SCREENTEX_COLOUR]->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, m_FlipImGuiImage ? 1.0f : 0.0f), ImVec2(1.0f, m_FlipImGuiImage ? 0.0f : 1.0f));
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Normal Texture"))
		{
			ImGui::Image(m_RenderManager->GetGBuffer()->m_ScreenTex[SCREENTEX_NORMALS]->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, m_FlipImGuiImage ? 1.0f : 0.0f), ImVec2(1.0f, m_FlipImGuiImage ? 0.0f : 1.0f));
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("PBR Texture"))
		{
			ImGui::Image(m_RenderManager->GetGBuffer()->m_ScreenTex[SCREENTEX_PBR]->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, m_FlipImGuiImage ? 1.0f : 0.0f), ImVec2(1.0f, m_FlipImGuiImage ? 0.0f : 1.0f));
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Position Texture"))
		{
			ImGui::Image(m_RenderManager->GetGBuffer()->m_ScreenTex[SCREENTEX_POSITION]->GetHandle(), ImVec2(128, 128), ImVec2(0.0f, m_FlipImGuiImage ? 1.0f : 0.0f), ImVec2(1.0f, m_FlipImGuiImage ? 0.0f : 1.0f));
			ImGui::TreePop();
		}

        auto entities = m_SceneManager->GetCurrentScene()->GetEntities();
        ImGui::Text("Number of Entities: %5.2i", static_cast<int>(entities.size()));

        if (ImGui::TreeNode("Enitities"))
        {
            for (auto& entity : entities)
            {
                if (ImGui::Selectable(entity->GetName().c_str(), m_Selected == entity.get()))
					m_Selected = entity.get();
            }
            ImGui::TreePop();
        }
    
        m_SceneManager->GetCurrentScene()->OnIMGUI();

		ImGui::End();

		if (m_Selected)
			m_Selected->OnIMGUI();

		OnGuizmo();
    }

    void Application::SetScene(Scene* scene)
    {
		scene->SetScreenWidth(m_Window->GetWidth());
		scene->SetScreenHeight(m_Window->GetHeight());
    }
}
