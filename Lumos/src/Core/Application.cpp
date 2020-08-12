#include "lmpch.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Engine.h"
#include "Editor/Editor.h"

#include "Graphics/API/Renderer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/RenderManager.h"
#include "Graphics/Layers/LayerStack.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/ShadowRenderer.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Renderers/SkyboxRenderer.h"

#include "Graphics/Layers/Layer2D.h"
#include "Graphics/Layers/Layer3D.h"

#include "Scene/Component/MeshComponent.h"

#include "Maths/Transform.h"

#include "Scene/EntityFactory.h"
#include "Utilities/LoadImage.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/Profiler.h"
#include "Core/VFS.h"

#include "Scripting/Lua/LuaManager.h"

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
        , m_SceneViewWidth(800)
        , m_SceneViewHeight(600)
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

		VFS::Get()->Mount("CoreShaders", root + "/Lumos/res/shaders");
		VFS::Get()->Mount("CoreMeshes", root + "/Lumos/res/meshes");
		VFS::Get()->Mount("CoreTextures", root + "/Lumos/res/textures");
		VFS::Get()->Mount("CoreFonts", root + "/Lumos/res/fonts");

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

		DebugRenderer::Init(screenWidth, screenHeight);
            
    #ifndef LUMOS_PLATFORM_IOS
        auto shadowRenderer = new Graphics::ShadowRenderer();
        auto shadowLayer = new Layer3D(shadowRenderer);
        Application::Get().GetRenderManager()->SetShadowRenderer(shadowRenderer);
        shadowLayer->OnAttach();
        m_LayerStack->PushLayer(shadowLayer);
    #endif
        
        auto layerSky = new Layer3D(new Graphics::SkyboxRenderer(screenWidth, screenHeight), "Skybox");
        auto layer2D = new Layer2D(new Graphics::Renderer2D(screenWidth, screenHeight, false, false, true));
        auto layerDeferred = new Layer3D(new Graphics::DeferredRenderer(screenWidth, screenHeight), "Deferred");
        m_LayerStack->PushLayer(layerDeferred);
        m_LayerStack->PushLayer(layerSky);
        m_LayerStack->PushLayer(layer2D);
        
        layerDeferred->OnAttach();
        layerSky->OnAttach();
        layer2D->OnAttach();
    
	}

	int Application::Quit(bool pause, const std::string& reason)
	{
		Material::ReleaseDefaultTexture();
		Engine::Release();
		Input::Release();
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

	Maths::Vector2 Application::GetWindowSize() const
	{
#ifdef LUMOS_EDITOR
		return Maths::Vector2(static_cast<float>(m_SceneViewWidth), static_cast<float>(m_SceneViewHeight));
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

			if(Input::GetInput()->GetKeyPressed(Lumos::InputCode::Key::Escape))
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

		m_SceneManager->ApplySceneSwitch();

		return m_CurrentState != AppState::Closing;
	}

	void Application::OnRender()
	{
		if(m_LayerStack->GetCount() > 0 || m_LayerStack->GetCount() > 0)
		{
			Graphics::Renderer::GetRenderer()->Begin();
			DebugRenderer::Reset();

			m_SystemManager->OnDebugDraw();

			m_LayerStack->OnRender(m_SceneManager->GetCurrentScene());
#ifdef LUMOS_EDITOR
			m_Editor->DebugDraw();
			m_Editor->OnRender();
#endif
			DebugRenderer::Render(m_SceneManager->GetCurrentScene(), nullptr, nullptr);
			m_ImGuiLayer->OnRender(m_SceneManager->GetCurrentScene());

			Graphics::Renderer::GetRenderer()->Present();
		}
	}

	void Application::OnUpdate(const TimeStep& dt)
	{

#ifdef LUMOS_EDITOR
		m_Editor->OnUpdate(dt);

		if(Application::Get().GetEditorState() != EditorState::Paused
			&& Application::Get().GetEditorState() != EditorState::Preview)
#endif
		{
			m_SystemManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
			LuaManager::Get().OnUpdate(m_SceneManager->GetCurrentScene());
			m_SceneManager->GetCurrentScene()->OnUpdate(dt);
		}

		if(!m_Minimized)
		{
			m_LayerStack->OnUpdate(dt, m_SceneManager->GetCurrentScene());
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
		m_SceneManager->GetCurrentScene()->OnImGui();
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
		DebugRenderer::OnResize(width, height);

		Graphics::GraphicsContext::GetContext()->WaitIdle();
	}
    
    void Application::EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName)
    {
        u32 width, height, bits;
        bool isHDR;
        auto texture = LoadImageFromFile(texFilePath.c_str(), &width, &height, &bits, &isHDR);
        
        size_t psize = width * height * 4;
        std::ofstream file;
        file.open(outPath);
        file << "//Generated by Lumos using " << texFilePath << std::endl;
		file << "static const u32 " << arrayName << "Width = " << width << ";" << std::endl;
		file << "static const u32 " << arrayName << "Height = " << height << ";" << std::endl;
		file << "static const u8 " << arrayName << "[] = {" << (int)texture[0];
		for (size_t i = 1; i< psize; ++i)
        file << "," << (int)texture[i];
        file << "};";
        
        file.close();
    }
}
