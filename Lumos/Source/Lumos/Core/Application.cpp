#include "Precompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
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
#include "Core/JobSystem.h"
#include "Core/StringUtilities.h"
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
        : m_Frames(0)
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
        VFS::Get()->Mount("Meshes", root + projectRoot + std::string("Assets/Meshes"));
        VFS::Get()->Mount("Textures", root + projectRoot + std::string("Assets/Textures"));
        VFS::Get()->Mount("Sounds", root + projectRoot + std::string("Assets/Sounds"));
        VFS::Get()->Mount("Scripts", root + projectRoot + std::string("Assets/Scripts"));
        VFS::Get()->Mount("Scenes", root + projectRoot + std::string("Assets/Scenes"));
        VFS::Get()->Mount("Assets", root + projectRoot + std::string("Assets"));
        VFS::Get()->Mount("CoreShaders", root + std::string("/Lumos/Assets/Shaders"));
#endif

        m_SceneManager = CreateUniqueRef<SceneManager>();

        Deserialise(FilePath);

        Engine::Get();

        m_Timer = CreateUniqueRef<Timer>();

        WindowProperties windowProperties;
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
        VFS::Get()->Mount("Meshes", projectRoot + std::string("Assets/meshes"));
        VFS::Get()->Mount("Textures", projectRoot + std::string("Assets/textures"));
        VFS::Get()->Mount("Sounds", projectRoot + std::string("Assets/sounds"));
        VFS::Get()->Mount("Scripts", projectRoot + std::string("Assets/scripts"));
        VFS::Get()->Mount("Scenes", projectRoot + std::string("Assets/scenes"));
        VFS::Get()->Mount("Assets", projectRoot + std::string("Assets"));
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

        uint32_t screenWidth = m_Window->GetWidth();
        uint32_t screenHeight = m_Window->GetHeight();
        m_SystemManager = CreateUniqueRef<SystemManager>();

        System::JobSystem::Context context;

        System::JobSystem::Execute(context, [](JobDispatchArgs args)
            { Lumos::Input::Get(); });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            {
                auto audioManager = AudioManager::Create();
                if(audioManager)
                {
                    audioManager->OnInit();
                    audioManager->SetPaused(true);
                    m_SystemManager->RegisterSystem<AudioManager>(audioManager);
                }
            });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            {
                m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
                m_SystemManager->RegisterSystem<B2PhysicsEngine>();
            });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            { m_SceneManager->LoadCurrentList(); });

        // Graphics Loading on main thread
        Graphics::Renderer::Init(screenWidth, screenHeight);

        m_ImGuiManager = CreateUniqueRef<ImGuiManager>(false);
        m_ImGuiManager->OnInit();
        m_ShaderLibrary = CreateRef<ShaderLibrary>();

        m_RenderGraph = CreateUniqueRef<Graphics::RenderGraph>(screenWidth, screenHeight);

        m_CurrentState = AppState::Running;

        Graphics::Material::InitDefaultTexture();

        //Need to disable shadows for A12 and earlier - doesn't support rendering to depth array
        m_RenderGraph->AddRenderer(new Graphics::ShadowRenderer());
        m_RenderGraph->AddRenderer(new Graphics::DeferredRenderer(screenWidth, screenHeight));
        m_RenderGraph->AddRenderer(new Graphics::SkyboxRenderer(screenWidth, screenHeight));
        m_RenderGraph->AddRenderer(new Graphics::Renderer2D(screenWidth, screenHeight, false, false, true));
        m_RenderGraph->EnableDebugRenderer(true);

        System::JobSystem::Wait(context);
    }

    void Application::Quit()
    {
        LUMOS_PROFILE_FUNCTION();
        Serialise(FilePath);
        Graphics::Material::ReleaseDefaultTexture();
        Engine::Release();
        Input::Release();

        m_ShaderLibrary.reset();
        m_SceneManager.reset();
        m_RenderGraph.reset();
        m_SystemManager.reset();
        m_ImGuiManager.reset();

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

    float Application::GetWindowDPI() const
    {
        return m_Window->GetDPIScale();
    }

    bool Application::OnFrame()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_FRAMEMARKER();

        if(m_SceneManager->GetSwitchingScene())
        {
            LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
            m_SceneManager->ApplySceneSwitch();
            return m_CurrentState != AppState::Closing;
        }

        float now = m_Timer->GetElapsedS();
        auto& stats = Engine::Get().Statistics();
        auto& ts = Engine::GetTimeStep();

        {
            LUMOS_PROFILE_SCOPE("Application::TimeStepUpdates");
            ts.Update(now);

            ImGuiIO& io = ImGui::GetIO();
            io.DeltaTime = ts.GetSeconds();

            stats.FrameTime = ts.GetMillis();
        }

        Input::Get().ResetPressed();
        m_Window->ProcessInput();

        if(Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape))
        {
            m_CurrentState = AppState::Closing;
        }

        //Exit frame early if escape or close button clicked
        //Prevents a crash with vulkan/moltenvk
        if(m_CurrentState == AppState::Closing)
            return false;

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

            DebugRenderer::Clear();
            Graphics::Renderer::GetRenderer()->Begin();

            OnRender();
            m_ImGuiManager->OnRender(m_SceneManager->GetCurrentScene());

            Graphics::Renderer::GetRenderer()->Present();

            m_Frames++;
        }

        {
            LUMOS_PROFILE_SCOPE("Application::UpdateGraphicsStats");
            stats.UsedGPUMemory = Graphics::GraphicsContext::GetContext()->GetGPUMemoryUsed();
            stats.TotalGPUMemory = Graphics::GraphicsContext::GetContext()->GetTotalGPUMemory();
        }

        {
            LUMOS_PROFILE_SCOPE("Application::WindowUpdate");
            m_Window->UpdateCursorImGui();
            m_Window->OnUpdate();
        }

        if(now - m_SecondTimer > 1.0f)
        {
            LUMOS_PROFILE_SCOPE("Application::FrameRateCalc");
            m_SecondTimer += 1.0f;

            stats.FramesPerSecond = m_Frames;
            stats.UpdatesPerSecond = m_Updates;

            m_Frames = 0;
            m_Updates = 0;
        }

        return m_CurrentState != AppState::Closing;
    }

    void Application::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        if(m_RenderGraph->GetCount() > 0)
        {
            m_RenderGraph->BeginScene(m_SceneManager->GetCurrentScene());

            m_RenderGraph->OnRender();
            OnDebugDraw();
            DebugRenderer::Render();
        }
    }

    void Application::OnDebugDraw()
    {
        m_SystemManager->OnDebugDraw();
    }

    void Application::OnUpdate(const TimeStep& dt)
    {
        LUMOS_PROFILE_FUNCTION();
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

        Input::Get().OnEvent(e);
    }

    void Application::Run()
    {
        while(OnFrame())
        {
        }

        Quit();
    }

    void Application::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
    }

    Ref<ShaderLibrary>& Application::GetShaderLibrary() { return m_ShaderLibrary; }

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

        Graphics::Renderer::GetRenderer()->OnResize(width, height);
        m_RenderGraph->OnResize(width, height);

        Graphics::GraphicsContext::GetContext()->WaitIdle();

        return false;
    }

    void Application::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        m_SceneManager->GetCurrentScene()->OnImGui();
    }

    void Application::OnSceneViewSizeUpdated(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        Graphics::GraphicsContext::GetContext()->WaitIdle();

        WindowResizeEvent e(width, height);
        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return;
        }
        m_Minimized = false;
        m_RenderGraph->OnResize(width, height);
        m_RenderGraph->OnEvent(e);

        Graphics::GraphicsContext::GetContext()->WaitIdle();
    }

    void Application::EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName)
    {
        uint32_t width, height, bits;
        bool isHDR;
        auto texture = LoadImageFromFile(texFilePath.c_str(), &width, &height, &bits, &isHDR);

        size_t psize = width * height * 4;
        std::ofstream file;
        file.open(outPath);
        file << "//Generated by Lumos using " << texFilePath << std::endl;
        file << "static const uint32_t " << arrayName << "Width = " << width << ";" << std::endl;
        file << "static const uint32_t " << arrayName << "Height = " << height << ";" << std::endl;
        file << "static const uint8_t " << arrayName << "[] = {" << (int)texture[0];
        for(size_t i = 1; i < psize; ++i)
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
                cereal::JSONOutputArchive output { storage };
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
