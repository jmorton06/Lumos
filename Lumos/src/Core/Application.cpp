#include "Precompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Scene/SceneManager.h"
#include "Engine.h"
#include "Utilities/Timer.h"

#include "Graphics/API/Renderer.h"
#include "Graphics/API/GraphicsContext.h"
#include "Graphics/Renderers/RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/Renderer2D.h"
#include "Graphics/Renderers/DeferredRenderer.h"
#include "Graphics/Renderers/ForwardRenderer.h"
#include "Graphics/Renderers/ShadowRenderer.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Renderers/SkyboxRenderer.h"

#include "Maths/Transform.h"

#include "Scene/EntityFactory.h"
#include "Utilities/LoadImage.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/OS/OS.h"
#include "Core/Profiler.h"
#include "Core/VFS.h"
#include "Core/OS/FileSystem.h"
#include "Scripting/Lua/LuaManager.h"
#include "ImGui/ImGuiManager.h"
#include "Events/ApplicationEvent.h"
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

#include <cereal/archives/json.hpp>
#include <imgui/imgui.h>
namespace Lumos
{
	Application* Application::s_Instance = nullptr;
	
	Application::Application(const std::string& projectRoot, const std::string& projectName)
		: m_UpdateTimer(0)
		, m_Frames(0)
		, m_Updates(0)
        , m_SceneViewWidth(800)
        , m_SceneViewHeight(600)
	{
		LUMOS_PROFILE_FUNCTION();
		LUMOS_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
        
#ifdef LUMOS_PLATFORM_IOS
        FilePath = Lumos::OS::Instance()->GetAssetPath() + projectName + ".lmproj";
#else
        FilePath = projectRoot + projectName + std::string(".lmproj");
#endif
        
#ifndef LUMOS_PLATFORM_IOS
        const std::string root = ROOT_DIR;
        VFS::Get()->Mount("Meshes", root + projectRoot + std::string("/res/meshes"));
        VFS::Get()->Mount("Textures", root + projectRoot +  std::string("/res/textures"));
        VFS::Get()->Mount("Sounds", root + projectRoot + std::string("/res/sounds"));
        VFS::Get()->Mount("Scripts", root + projectRoot + std::string("/res/scripts"));
        VFS::Get()->Mount("Scenes", root + projectRoot + std::string("/res/scenes"));
        VFS::Get()->Mount("CoreShaders", root + std::string("/Lumos/res/EngineShaders"));
#endif
		
        m_SceneManager = CreateUniqueRef<SceneManager>();

		Deserialise(FilePath);

		Engine::Get();

		m_Timer = CreateUniqueRef<Timer>();
        
		WindowProperties  windowProperties;
		windowProperties.Width = Width;
		windowProperties.Height = Height;
		windowProperties.RenderAPI = RenderAPI;
		windowProperties.Fullscreen = Fullscreen;
		windowProperties.Borderless = Borderless;
		windowProperties.ShowConsole = ShowConsole;
		windowProperties.Title = Title;
		windowProperties.VSync = VSync;
		
		m_Window = UniqueRef<Window>(Window::Create(windowProperties));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
        
        m_EditorState = EditorState::Play;

		ImGui::CreateContext();
		ImGui::StyleColorsDark();
	}

	Application::~Application()
	{
		LUMOS_PROFILE_FUNCTION();
		ImGui::DestroyContext();
	}
	
	void Application::OpenProject(const std::string& filePath)
	{
        FilePath = filePath;
        
#ifndef LUMOS_PLATFORM_IOS
		auto projectRoot = StringUtilities::GetFileLocation(filePath);
        VFS::Get()->Mount("Meshes",  projectRoot + std::string("/res/meshes"));
        VFS::Get()->Mount("Textures", projectRoot +  std::string("/res/textures"));
        VFS::Get()->Mount("Sounds", projectRoot + std::string("/res/sounds"));
        VFS::Get()->Mount("Scripts", projectRoot + std::string("/res/scripts"));
        VFS::Get()->Mount("Scenes", projectRoot + std::string("/res/scenes"));
		#endif
	}

	Scene* Application::GetCurrentScene() const
	{
		LUMOS_PROFILE_FUNCTION();
		return m_SceneManager->GetCurrentScene();
	}

	void Application::Init()
	{
		LUMOS_PROFILE_FUNCTION();
		// Initialise the Window
		if(!m_Window->HasInitialised())
			Quit();

		u32 screenWidth = m_Window->GetWidth();
		u32 screenHeight = m_Window->GetHeight();

		Lumos::Input::Create();
        
        m_ShaderLibrary = CreateRef<ShaderLibrary>();
        
		Graphics::Renderer::Init(screenWidth, screenHeight);

		// Graphics Loading on main thread
		m_RenderGraph = CreateUniqueRef<Graphics::RenderGraph>(screenWidth, screenHeight);

        m_ImGuiManager = new ImGuiManager(false);
        m_ImGuiManager->OnInit();

		m_SystemManager = CreateUniqueRef<SystemManager>();

		auto audioManager = AudioManager::Create();
		if(audioManager)
		{
			audioManager->OnInit();
			m_SystemManager->RegisterSystem<AudioManager>(audioManager);
		}

		m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
		m_SystemManager->RegisterSystem<B2PhysicsEngine>();
		
		Application::Get().GetSystem<LumosPhysicsEngine>()->SetPaused(false);
		Application::Get().GetSystem<B2PhysicsEngine>()->SetPaused(false);
        
		Graphics::Material::InitDefaultTexture();
        
        m_SceneManager->LoadCurrentList();

		m_CurrentState = AppState::Running;

		DebugRenderer::Init(screenWidth, screenHeight);
            
//#ifndef LUMOS_PLATFORM_IOS //Need to disable for A12 and earlier
        auto shadowRenderer = new Graphics::ShadowRenderer();
        Application::Get().GetRenderGraph()->SetShadowRenderer(shadowRenderer);
        m_RenderGraph->AddRenderer(shadowRenderer);
//#endif
        
        m_RenderGraph->AddRenderer(new Graphics::DeferredRenderer(screenWidth, screenHeight));
        m_RenderGraph->AddRenderer(new Graphics::SkyboxRenderer(screenWidth, screenHeight));
        m_RenderGraph->AddRenderer(new Graphics::Renderer2D(screenWidth, screenHeight, false, false, true));
	}

	void Application::Quit()
	{
		LUMOS_PROFILE_FUNCTION();
		Serialise(FilePath);
		Graphics::Material::ReleaseDefaultTexture();
		Engine::Release();
		Input::Release();
		DebugRenderer::Release();

		m_ShaderLibrary.reset();
		m_SceneManager.reset();
		m_RenderGraph.reset();
		m_SystemManager.reset();

		delete m_ImGuiManager;
        
		Graphics::Renderer::Release();
        Graphics::Pipeline::ClearCache();
        Graphics::RenderPass::ClearCache();
        Graphics::Framebuffer::ClearCache();
		
        m_Window.reset();
	}

	Maths::Vector2 Application::GetWindowSize() const
	{
		return Maths::Vector2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
	}

	bool Application::OnFrame()
	{
		LUMOS_PROFILE_FUNCTION();
		float now = m_Timer->GetElapsedS();

#ifdef LUMOS_LIMIT_FRAMERATE
		if(now - m_UpdateTimer > Engine::Get().TargetFrameRate())
		{
			m_UpdateTimer += Engine::Get().TargetFrameRate();
#endif
				auto& stats = Engine::Get().Statistics();
				auto& ts = Engine::GetTimeStep();
			
			{
				LUMOS_PROFILE_SCOPE("Application::TimeStepUpdates");
				ts.Update(now);
				
				ImGuiIO& io = ImGui::GetIO();
				io.DeltaTime = ts.GetMillis();
				
				stats.FrameTime = ts.GetMillis();
			}
			
			{
				LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
                m_SceneManager->ApplySceneSwitch();
			}
			{
				LUMOS_PROFILE_SCOPE("Application::ImGui::NewFrame");
				ImGui::NewFrame();
			}

			{
				LUMOS_PROFILE_SCOPE("Application::Update");
				OnUpdate(ts);
				m_Updates++;
			}

			if(!m_Minimized)
			{
				LUMOS_PROFILE_SCOPE("Application::Render");
				OnRender();
                Graphics::Renderer::GetRenderer()->Present();
				m_Frames++;
			}
			
			{
				LUMOS_PROFILE_SCOPE("Application::UpdateGraphicsStats");
                stats.UsedGPUMemory = Graphics::GraphicsContext::GetContext()->GetGPUMemoryUsed();
				stats.TotalGPUMemory = Graphics::GraphicsContext::GetContext()-> GetTotalGPUMemory();
			}
			{
				LUMOS_PROFILE_SCOPE("Application::WindowUpdate");
                Input::GetInput()->ResetPressed();
                m_Window->UpdateCursorImGui();
				m_Window->OnUpdate();
			}

			if(Input::GetInput()->GetKeyPressed(Lumos::InputCode::Key::Escape))
				m_CurrentState = AppState::Closing;
#ifdef LUMOS_LIMIT_FRAMERATE
		}
#endif
		
		if(m_Timer->GetElapsedS() - m_SecondTimer > 1.0f)
		{
			LUMOS_PROFILE_SCOPE("Application::FrameRateCalc");

			m_SecondTimer += 1.0f;
			
			stats.FramesPerSecond = m_Frames;
			stats.UpdatesPerSecond = m_Updates;

			m_Frames = 0;
			m_Updates = 0;
			
			m_SceneManager->GetCurrentScene()->OnTick();
		}
		
		LUMOS_PROFILE_FRAMEMARKER();

		return m_CurrentState != AppState::Closing;
	}

	void Application::OnRender()
	{
		LUMOS_PROFILE_FUNCTION();
		if(m_RenderGraph->GetCount() > 0)
		{
			Graphics::Renderer::GetRenderer()->Begin();
			DebugRenderer::Reset();

			m_SystemManager->OnDebugDraw();

			m_RenderGraph->OnRender(m_SceneManager->GetCurrentScene());
            
			DebugRenderer::Render(m_SceneManager->GetCurrentScene(), nullptr, nullptr);
            m_ImGuiManager->OnRender(m_SceneManager->GetCurrentScene());
		}
	}

	void Application::OnUpdate(const TimeStep& dt)
	{
		if(Application::Get().GetEditorState() != EditorState::Paused
			&& Application::Get().GetEditorState() != EditorState::Preview)
		{
			m_SystemManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
			LuaManager::Get().OnUpdate(m_SceneManager->GetCurrentScene());
			m_SceneManager->GetCurrentScene()->OnUpdate(dt);
		}

		if(!m_Minimized)
		{
			m_RenderGraph->OnUpdate(dt, m_SceneManager->GetCurrentScene());
		}
        m_ImGuiManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
        
        {
            //Do every few frames instead?
            Graphics::Pipeline::DeleteUnusedCache();
            Graphics::RenderPass::DeleteUnusedCache();
            Graphics::Framebuffer::DeleteUnusedCache();
        }
        m_ShaderLibrary->Update(dt.GetElapsedMillis());
	}

	void Application::OnEvent(Event& e)
	{
		LUMOS_PROFILE_FUNCTION();
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        m_ImGuiManager->OnEvent(e);
		if(e.Handled())
			return;
		m_RenderGraph->OnEvent(e);

		if(e.Handled())
			return;

		m_SceneManager->GetCurrentScene()->OnEvent(e);

		Input::GetInput()->OnEvent(e);
	}

	void Application::Run()
	{
		m_UpdateTimer = m_Timer->GetElapsedS();
		while(OnFrame())
		{
		}

		Quit();
	}

	void Application::OnNewScene(Scene* scene)
	{
		LUMOS_PROFILE_FUNCTION();
	}

	void Application::OnExitScene()
	{
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_CurrentState = AppState::Closing;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		LUMOS_PROFILE_FUNCTION();
		Graphics::GraphicsContext::GetContext()->WaitIdle();

		int width = e.GetWidth(), height = e.GetHeight();

		if(width == 0 || height == 0)
		{
			m_Minimized = true;
			return false;
		}
		m_Minimized = false;

		m_RenderGraph->OnResize(width, height);
		Graphics::Renderer::GetRenderer()->OnResize(width, height);
		DebugRenderer::OnResize(width, height);

		Graphics::GraphicsContext::GetContext()->WaitIdle();

		return false;
	}

	void Application::OnImGui()
	{
		LUMOS_PROFILE_FUNCTION();
		m_SceneManager->GetCurrentScene()->OnImGui();
	}

	void Application::OnSceneViewSizeUpdated(u32 width, u32 height)
	{
		LUMOS_PROFILE_FUNCTION();
		Graphics::GraphicsContext::GetContext()->WaitIdle();

		WindowResizeEvent e(width, height);
		if(width == 0 || height == 0)
		{
			m_Minimized = true;
		}
		m_Minimized = false;
		m_RenderGraph->OnResize(width, height);
		m_RenderGraph->OnEvent(e);
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
	
	void Application::Serialise(const std::string& filePath)
	{
		LUMOS_PROFILE_FUNCTION();
		{
			std::stringstream storage;
			{
				// output finishes flushing its contents when it goes out of scope
				cereal::JSONOutputArchive output{storage};
				output(*this);
			}
			auto fullPath = ROOT_DIR + filePath;
			FileSystem::WriteTextFile(fullPath, storage.str());
		}
	}
	
	void Application::Deserialise(const std::string& filePath)
	{
		LUMOS_PROFILE_FUNCTION();
		{
#ifdef LUMOS_PLATFORM_IOS
            auto fullPath = filePath;
#else
            auto fullPath = ROOT_DIR + filePath;
#endif
			if(!FileSystem::FileExists(fullPath))
			{
                LUMOS_LOG_INFO("No saved Project file found {0}", fullPath);
				{
					//Set Default values
					RenderAPI = 1;
					Width = 1200;
					Height = 800;
					Borderless = false;
					VSync = false;
					Title = "LumosGame";
					ShowConsole = false;
					Fullscreen = false;
				}
				return;
			}
			
  			std::string data = FileSystem::ReadTextFile(fullPath);
			std::istringstream istr;
			istr.str(data);
			cereal::JSONInputArchive input(istr);
			input(*this);
	}
	}
}
	
