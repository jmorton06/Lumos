#include "LM.h"
#include "Application.h"

#include "Scene.h"
#include "SceneManager.h"
#include "Input.h"
#include "Engine.h"
#include "Window.h"
#include "Editor/Editor.h"

#include "Graphics/API/Renderer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/API/Texture.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Layers/ImGuiLayer.h"
#include "Graphics/Camera/Camera.h"

#include "ECS/EntityManager.h"
#include "ECS/ComponentManager.h"

#include "Utilities/CommonUtils.h"
#include "Utilities/AssetsManager.h"
#include "Core/VFS.h"
#include "Core/JobSystem.h"
#include "Core/OS/Thread.h"
#include "Scripting/LuaScript.h"

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
		: m_UpdateTimer(0), m_Frames(0), m_Updates(0)
	{
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

#ifdef  LUMOS_EDITOR
		m_Editor = lmnew Editor(this, properties.Width, properties.Height);
#endif
		Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(properties.RenderAPI));

		Engine::Instance();
		EntityManager::Instance();
		ComponentManager::Instance();

		m_Timer = CreateScope<Timer>();

		const String root = ROOT_DIR;

		VFS::Get()->Mount("CoreShaders", root + "/lumos/res/shaders");
		VFS::Get()->Mount("CoreMeshes", root + "/lumos/res/meshes");
		VFS::Get()->Mount("CoreTextures", root + "/lumos/res/textures");
		VFS::Get()->Mount("CoreFonts", root + "/lumos/res/fonts");

		m_Window = Scope<Window>(Window::Create(properties));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		m_SceneManager = CreateScope<SceneManager>();

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

        Thread* thread = Thread::Create(nullptr, this);
        delete thread;
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

		u32 screenWidth = m_Window->GetWidth();
		u32 screenHeight = m_Window->GetHeight();

		Lumos::Input::Create();

		Graphics::GraphicsContext::GetContext()->Init();

#ifdef  LUMOS_EDITOR
		m_Editor->OnInit();
#endif

		Graphics::Renderer::Init(screenWidth, screenHeight);

		//Graphics Loading on main thread
		AssetsManager::InitializeMeshes();
		m_RenderManager = CreateScope<Graphics::RenderManager>(screenWidth, screenHeight);

		m_LayerStack = lmnew LayerStack();
		PushLayerInternal(lmnew ImGuiLayer(false),true,false);

		m_SystemManager = CreateScope<SystemManager>();
		m_SystemManager->RegisterSystem<AudioManager>(AudioManager::Create());
		m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
		m_SystemManager->RegisterSystem<B2PhysicsEngine>();

		m_SystemManager->GetSystem<AudioManager>()->OnInit();

		m_CurrentState = AppState::Running;
	}

	int Application::Quit(bool pause, const std::string &reason)
	{
		Engine::Release();
		Input::Release();
		AssetsManager::ReleaseMeshes();
		EntityManager::Release();
		ComponentManager::Release();
		SoundManager::Release();
		LuaScript::Release();

		m_SceneManager.reset();
		m_RenderManager.reset();
		m_SystemManager.reset();

		delete m_LayerStack;

		Graphics::Renderer::Release();
		Graphics::GraphicsContext::Release();

		if (pause)
		{
			LUMOS_CORE_ERROR("{0}", reason);
		}

		return 0;
	}

	void Application::SetActiveCamera(Camera* camera)
	{
		m_ActiveCamera = camera;
	}

	Maths::Vector2 Application::GetWindowSize() const
	{
		return Maths::Vector2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
	}

	bool Application::OnFrame()
	{
		float now = m_Timer->GetMS(1.0f) * 1000.0f;

#ifdef LUMOS_LIMIT_FRAMERATE
		if (now - m_UpdateTimer > Engine::Instance()->TargetFrameRate())
		{
			m_UpdateTimer += Engine::Instance()->TargetFrameRate();
#endif

            Engine::GetTimeStep()->Update(now);

			{
				OnUpdate(Engine::GetTimeStep());
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


		m_SceneManager->ApplySceneSwitch();

		return m_CurrentState != AppState::Closing;
	}

	void Application::OnRender()
	{
		if (m_LayerStack->GetCount() > 0)
		{
			Graphics::Renderer::GetRenderer()->Begin();

			m_LayerStack->OnRender(m_SceneManager->GetCurrentScene());

			Graphics::Renderer::GetRenderer()->Present();
		}
	}

	void Application::OnUpdate(TimeStep* dt)
	{
		const u32 sceneIdx = m_SceneManager->GetCurrentSceneIndex();
		const u32 sceneMax = m_SceneManager->SceneCount();

		if (Input::GetInput().GetKeyPressed(InputCode::Key::P)) Application::Instance()->GetSystem<LumosPhysicsEngine>()->SetPaused(!Application::Instance()->GetSystem<LumosPhysicsEngine>()->IsPaused());
		if (Input::GetInput().GetKeyPressed(InputCode::Key::P)) Application::Instance()->GetSystem<B2PhysicsEngine>()->SetPaused(!Application::Instance()->GetSystem<B2PhysicsEngine>()->IsPaused());

		if (Input::GetInput().GetKeyPressed(InputCode::Key::J)) CommonUtils::AddSphere(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(InputCode::Key::K)) CommonUtils::AddPyramid(m_SceneManager->GetCurrentScene());
		if (Input::GetInput().GetKeyPressed(InputCode::Key::L)) CommonUtils::AddLightCube(m_SceneManager->GetCurrentScene());

		if (Input::GetInput().GetKeyPressed(InputCode::Key::E)) m_SceneManager->SwitchScene((sceneIdx + 1) % sceneMax);
		if (Input::GetInput().GetKeyPressed(InputCode::Key::Q)) m_SceneManager->SwitchScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);
		if (Input::GetInput().GetKeyPressed(InputCode::Key::R)) m_SceneManager->SwitchScene(sceneIdx);
		if (Input::GetInput().GetKeyPressed(InputCode::Key::V)) m_Window->ToggleVSync();

#ifdef LUMOS_EDITOR
		if (Application::Instance()->GetEditorState() != EditorState::Paused && Application::Instance()->GetEditorState() != EditorState::Preview)
#endif
		{
			m_SceneManager->GetCurrentScene()->OnUpdate(Engine::GetTimeStep());
			m_SystemManager->OnUpdate(Engine::GetTimeStep());
		}

		m_LayerStack->OnUpdate(Engine::GetTimeStep(), m_SceneManager->GetCurrentScene());
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

	void Application::OnNewScene(Scene * scene)
	{
#ifdef LUMOS_EDITOR
		m_Editor->OnNewScene(scene);
#endif
	}

	void Application::OnExitScene()
	{
		for (auto& layer : m_CurrentSceneLayers)
		{
			m_LayerStack->PopLayer(layer);
		}

		m_CurrentSceneLayers.clear();
	}

	void Application::PushLayerInternal(Layer* layer, bool overlay, bool sceneAdded)
	{
		if(overlay)
			m_LayerStack->PushOverlay(layer);
		else
			m_LayerStack->PushLayer(layer);

		layer->OnAttach();

		if (sceneAdded)
			m_CurrentSceneLayers.emplace_back(layer);
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

	bool Application::OnWindowResize(WindowResizeEvent &e)
	{
		auto windowSize = GetWindowSize();

		u32 width = 1;
		u32 height = 1;

		if (e.GetWidth() != 0) width = e.GetWidth();
		if (e.GetHeight() != 0) height = e.GetHeight();

		m_RenderManager->OnResize(width, height);
		Graphics::Renderer::GetRenderer()->OnResize(width, height);
		return false;
	}

	void Application::OnImGui()
	{
#ifdef LUMOS_EDITOR
		if (m_AppType == AppType::Editor)
			m_Editor->OnImGui();
#endif
	}
}
