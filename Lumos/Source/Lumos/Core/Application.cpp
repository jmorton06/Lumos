#include "Precompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Engine.h"
#include "Utilities/Timer.h"

#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/Renderers/SceneRenderer.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/GridRenderer.h"
#include "Graphics/Font.h"
#include "Graphics/UI.h"
#include "Maths/Transform.h"
#include "Maths/MathsUtilities.h"
#include "Scene/EntityFactory.h"
#include "Utilities/LoadImage.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/Profiler.h"
#include "Core/OS/FileSystem.h"
#include "Core/JobSystem.h"
#include "Core/CoreSystem.h"
#include "Utilities/StringUtilities.h"
#include "Core/OS/FileSystem.h"
#include "Core/String.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/CommandLine.h"
#include "Core/Asset/AssetManager.h"
#include "Scripting/Lua/LuaManager.h"
#include "ImGui/ImGuiManager.h"
#include "Events/ApplicationEvent.h"
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"
#include "Embedded/EmbedAsset.h"
#include "Core/Asset/AssetRegistry.h"
#include "Core/DataStructures/Map.h"
#include "Core/Function.h"

#define SERIALISATION_INCLUDE_ONLY
#include "Scene/Serialisation/SerialisationImplementation.h"
#include "Scene/Serialisation/SerialiseApplication.h"

#include "Embedded/splash.inl"
#include "Core/OS/OS.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include <imgui/imgui.h>
#include <imgui/Plugins/implot/implot.h>

namespace Lumos
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
        : m_Frames(0)
        , m_Updates(0)
        , m_SceneViewWidth(800)
        , m_SceneViewHeight(600)
    {
        LUMOS_PROFILE_FUNCTION();
        ASSERT(!s_Instance, "Application already exists!");

        s_Instance = this;
    }

    Application::~Application()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::DestroyContext();
        ImPlot::DestroyContext();
    }

    static i32 EmbedShaderCount = 0;
    void EmbedShaderFunc(const char* path)
    {
        auto extension = StringUtilities::GetFilePathExtension(path);
        if(extension == "spv")
        {
            EmbedShader(path);
            EmbedShaderCount++;
        }
    };
    void Application::Init()
    {
        LUMOS_PROFILE_FUNCTION();
        m_FrameArena = ArenaAlloc(Megabytes(1));
        m_Arena      = ArenaAlloc(Kilobytes(64));

        m_EventQueue.Reserve(16);

        SetMaxImageDimensions(2048, 2048);

        m_SceneManager = CreateUniquePtr<SceneManager>();
        m_AssetManager = CreateSharedPtr<AssetManager>();

        Deserialise();

        CommandLine* cmdline = Internal::CoreSystem::GetCmdLine();
        if(cmdline->OptionBool(Str8Lit("help")))
        {
            LINFO("Print this help.\n Option 1 : EnableVulkanValidation");
        }

        Engine::Get();
        LuaManager::Get().OnInit();
        LuaManager::Get().OnNewProject(m_ProjectSettings.m_ProjectRoot);
        m_Timer = CreateUniquePtr<Timer>();

        Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(m_ProjectSettings.RenderAPI));

        WindowDesc windowDesc;
        windowDesc.Width       = m_ProjectSettings.Width;
        windowDesc.Height      = m_ProjectSettings.Height;
        windowDesc.RenderAPI   = m_ProjectSettings.RenderAPI;
        windowDesc.Fullscreen  = m_ProjectSettings.Fullscreen;
        windowDesc.Borderless  = m_ProjectSettings.Borderless;
        windowDesc.ShowConsole = m_ProjectSettings.ShowConsole;
        windowDesc.Title       = Str8StdS(m_ProjectSettings.Title);
        windowDesc.VSync       = m_ProjectSettings.VSync;

        if(m_ProjectSettings.DefaultIcon)
        {
            windowDesc.IconPaths = { Str8Lit("//Assets/Textures/icon.png"), Str8Lit("//Assets/Textures/icon32.png") };
        }

        // Initialise the Window
        m_Window = UniquePtr<Window>(Window::Create(windowDesc));
        if(!m_Window->HasInitialised())
            OnQuit();

        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        m_EditorState = EditorState::Play;

        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui::StyleColorsDark();

        bool loadEmbeddedShaders = true;
        if(FileSystem::FolderExists(m_ProjectSettings.m_EngineAssetPath + "Shaders"))
            loadEmbeddedShaders = false;

        if(!loadEmbeddedShaders)
        {
            ArenaTemp temp     = ScratchBegin(0, 0);
            String8 shaderPath = PushStr8F(temp.arena, "%sShaders/CompiledSPV/", m_ProjectSettings.m_EngineAssetPath.c_str());

            FileSystem::IterateFolder((const char*)shaderPath.str, EmbedShaderFunc);
            ScratchEnd(temp);

            LINFO("Embedded %i shaders.", EmbedShaderCount);
        }
        Graphics::Renderer::Init(loadEmbeddedShaders, m_ProjectSettings.m_EngineAssetPath);

        if(m_ProjectSettings.Fullscreen)
            m_Window->Maximise();

        // Draw Splash Screen
        {
            auto desc          = Graphics::TextureDesc(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR, Graphics::TextureWrap::REPEAT);
            desc.flags         = Graphics::TextureFlags::Texture_Sampled;
            auto splashTexture = Graphics::Texture2D::CreateFromSource(splashWidth, splashHeight, (void*)splash, desc);
            Graphics::Renderer::GetRenderer()->Begin();
            Graphics::Renderer::GetRenderer()->DrawSplashScreen(splashTexture);
            Graphics::Renderer::GetRenderer()->Present();
            // To Display the window
            m_Window->ProcessInput();
            m_Window->OnUpdate();

            delete splashTexture;
        }

        uint32_t screenWidth  = m_Window->GetWidth();
        uint32_t screenHeight = m_Window->GetHeight();
        m_SystemManager       = CreateUniquePtr<SystemManager>();

        System::JobSystem::Context context;

        System::JobSystem::Execute(context, [](JobDispatchArgs args)
                                   { Lumos::Input::Get(); });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
                                   {
                                       auto audioManager = AudioManager::Create();
                                       if (audioManager)
                                       {
                                           m_SystemManager->RegisterSystem<AudioManager>(audioManager);
                                       } });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
                                   {
                                       m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
                                       m_SystemManager->RegisterSystem<B2PhysicsEngine>();
                                       LINFO("Initialised Physics Manager"); });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
                                   { m_SceneManager->LoadCurrentList(); });

        m_ImGuiManager = CreateUniquePtr<ImGuiManager>(false);
        m_ImGuiManager->OnInit();
        LINFO("Initialised ImGui Manager");

        m_SceneRenderer = CreateUniquePtr<Graphics::SceneRenderer>(screenWidth, screenHeight);
        
        LINFO("Initialised SceneRenderer");

        LINFO("Waiting for Context...");
        System::JobSystem::Wait(context);

        LINFO("App is Running");
        m_CurrentState = AppState::Running;

        Graphics::Material::InitDefaultTexture();
        LINFO("Init Default Texture");
        
        Graphics::Font::InitDefaultFont();
        LINFO("Init Default Font");
        
        m_SceneRenderer->EnableDebugRenderer(true);
        LINFO("Debug Renderer Enabled");

#ifdef LUMOS_SSE
        LINFO("SSE Maths Enabled");
#endif

        LINFO("UI Arena Alloc");
        m_UIArena = ArenaAlloc(Megabytes(8));
        
        LINFO("Initializing Arena...");
        InitialiseUI(m_UIArena);
        
        LINFO("Getting DPI Scale");
        GetUIState()->DPIScale = Application::Get().GetWindow()->GetDPIScale();
        
        LINFO("Initalised UI");
        Maths::TestMaths();
    }

    void Application::OnQuit()
    {
        LUMOS_PROFILE_FUNCTION();
        Serialise();

        ArenaRelease(m_FrameArena);
        ArenaRelease(m_Arena);

        ShutDownUI();
        ArenaRelease(m_UIArena);

        Graphics::Material::ReleaseDefaultTexture();
        Graphics::Font::ShutdownDefaultFont();
        Engine::Release();
        Input::Release();

        m_AssetManager.reset();
        m_SceneManager.reset();
        m_SceneRenderer.reset();
        m_SystemManager.reset();
        m_ImGuiManager.reset();
        LuaManager::Release();

        Graphics::Pipeline::ClearCache();
        Graphics::RenderPass::ClearCache();
        Graphics::Framebuffer::ClearCache();

        m_Window.reset();

        Graphics::Renderer::Release();
    }

    void Application::OpenProject(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectSettings.m_ProjectName = StringUtilities::GetFileName(filePath);
        m_ProjectSettings.m_ProjectName = StringUtilities::RemoveFilePathExtension(m_ProjectSettings.m_ProjectName);

#ifndef LUMOS_PLATFORM_IOS
        auto projectRoot                = StringUtilities::GetFileLocation(filePath);
        m_ProjectSettings.m_ProjectRoot = projectRoot;

        String8 pathCopy                = PushStr8Copy(m_FrameArena, projectRoot.c_str());
        pathCopy                        = StringUtilities::ResolveRelativePath(m_FrameArena, pathCopy);
        m_ProjectSettings.m_ProjectRoot = (const char*)pathCopy.str;
#endif

        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Materials");

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
        m_SceneManager = CreateUniquePtr<SceneManager>();

        Deserialise();

        m_SceneManager->LoadCurrentList();
        m_SceneManager->ApplySceneSwitch();

        LuaManager::Get().OnNewProject(m_ProjectSettings.m_ProjectRoot);
    }

    void Application::OpenNewProject(const std::string& path, const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectSettings.m_ProjectRoot = path + name + "/";
        m_ProjectSettings.m_ProjectName = name;

        String8 pathCopy                = PushStr8Copy(m_FrameArena, m_ProjectSettings.m_ProjectRoot.c_str());
        pathCopy                        = StringUtilities::ResolveRelativePath(m_FrameArena, pathCopy);
        m_ProjectSettings.m_ProjectRoot = (const char*)pathCopy.str;

        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot);
        m_SceneManager = CreateUniquePtr<SceneManager>();

        MountFileSystemPaths();
        // Set Default values
        m_ProjectSettings.RenderAPI   = 1;
        m_ProjectSettings.Width       = 1200;
        m_ProjectSettings.Height      = 800;
        m_ProjectSettings.Borderless  = false;
        m_ProjectSettings.VSync       = true;
        m_ProjectSettings.Title       = "App";
        m_ProjectSettings.ShowConsole = false;
        m_ProjectSettings.Fullscreen  = false;

#ifdef LUMOS_PLATFORM_MACOS
        // This is assuming Application in bin/Release-macos-x86_64/LumosEditor.app
        LINFO(StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()).c_str());
        m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../../../Lumos/Assets/";
#else
        m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../Lumos/Assets/";
#endif

        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Scripts");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Textures");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Meshes");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Sounds");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs");
        FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Materials");

        MountFileSystemPaths();

        m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
        m_SceneManager->SwitchScene(0);

        // Set Default values
        m_ProjectSettings.Title      = "App";
        m_ProjectSettings.Fullscreen = false;

        m_SceneManager->ApplySceneSwitch();

        m_ProjectLoaded = true;

        Serialise();

        LuaManager::Get().OnNewProject(m_ProjectSettings.m_ProjectRoot);
    }

    void Application::MountFileSystemPaths()
    {
        FileSystem::Get().SetAssetRoot(PushStr8F(m_Arena, "%sAssets", m_ProjectSettings.m_ProjectRoot.c_str()));
    }

    Scene* Application::GetCurrentScene() const
    {
        LUMOS_PROFILE_FUNCTION_LOW();
        return m_SceneManager->GetCurrentScene();
    }

    Vec2 Application::GetWindowSize() const
    {
        if(!m_Window)
            return Vec2(0.0f, 0.0f);
        return Vec2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
    }

    float Application::GetWindowDPI() const
    {
        if(!m_Window)
            return 1.0f;

        return m_Window->GetDPIScale();
    }

    bool Application::OnFrame()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_FRAMEMARKER();

        ArenaClear(m_FrameArena);

        if(m_SceneManager->GetSwitchingScene())
        {
            LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
            Graphics::Renderer::GetGraphicsContext()->WaitIdle();
            m_SceneManager->ApplySceneSwitch();
            return m_CurrentState != AppState::Closing;
        }

        double now  = m_Timer->GetElapsedSD();
        auto& stats = Engine::Get().Statistics();
        auto& ts    = Engine::GetTimeStep();

        static int s_NumContiguousLargeFrames = 0;
        const int maxContiguousLargeFrames    = 2;

        if(ts.GetSeconds() > 5)
        {
            LWARN("Large frame time %.2f", ts.GetSeconds());

            s_NumContiguousLargeFrames++;
#ifdef LUMOS_DISABLE_LARGE_FRAME_TIME
            // Added to stop application locking computer
            // Exit if frametime exceeds 5 seconds
            return false;
#endif

            if(s_NumContiguousLargeFrames > maxContiguousLargeFrames)
                return false;
        }
        else
            s_NumContiguousLargeFrames = 0;

        ExecuteMainThreadQueue();

        {
            LUMOS_PROFILE_SCOPE("Application::TimeStepUpdates");
            ts.OnUpdate();

            ImGuiIO& io  = ImGui::GetIO();
            io.DeltaTime = (float)ts.GetSeconds();

            stats.FrameTime = ts.GetMillis();
        }

        // Process Input events before ImGui::NewFrame
        Input::Get().ResetPressed();
        m_Window->ProcessInput();

        {
            LUMOS_PROFILE_SCOPE("ImGui::NewFrame");
            ImGui::NewFrame();
        }

        Vec2 frameSize = { (float)m_SceneViewWidth, (float)m_SceneViewHeight }; // GetWindowSize();
        UIBeginFrame(frameSize, (float)ts.GetSeconds(), m_SceneViewPosition);
        UIBeginBuild();

        static bool showTestUI = false;
        if(Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape))
            showTestUI = !showTestUI;
        if(showTestUI)
            TestUI();

        {
            std::scoped_lock<std::mutex> lock(m_EventQueueMutex);

            for(auto& event : m_EventQueue)
            {
                event();
            }
            m_EventQueue.Clear();

            // Process custom event queue
            while(!m_EventQueue.Empty())
            {
                auto& func = m_EventQueue.Back();
                func();
                m_EventQueue.PopBack();
            }
        }

        System::JobSystem::Context context;

        {
            LUMOS_PROFILE_SCOPE("Application::Update");
            OnUpdate(ts);

            System::JobSystem::Execute(context, [](JobDispatchArgs args)
                                       { Application::UpdateSystems(); });
            m_Updates++;
        }

        // Exit frame early if escape or close button clicked
        // Prevents a crash with vulkan/moltenvk
        if(m_CurrentState == AppState::Closing)
        {
            System::JobSystem::Wait(context);
            return false;
        }

        UIEndFrame(Graphics::Font::GetDefaultFont());

        UIEndBuild();
        UILayout();
        // if (Platform->WindowIsActive)
        {
            // UIProcessInteractions();
        }
        UIAnimate();

        if(!m_Minimized)
        {
            LUMOS_PROFILE_SCOPE("Application::Render");
            Engine::Get().ResetStats();

            Graphics::Renderer::GetRenderer()->Begin();
            OnRender();
            m_ImGuiManager->OnNewFrame();
            m_ImGuiManager->OnRender(m_SceneManager->GetCurrentScene());

            // Clears debug line and point lists
            DebugRenderer::Reset((float)ts.GetSeconds());

            Graphics::Pipeline::DeleteUnusedCache();
            Graphics::Framebuffer::DeleteUnusedCache();
            Graphics::RenderPass::DeleteUnusedCache();

            m_AssetManager->Update((float)ts.GetElapsedSeconds());
            m_Frames++;
        }
        else
        {
            ImGui::Render();
        }

        {
            LUMOS_PROFILE_SCOPE("Application::UpdateGraphicsStats");
            stats.UsedGPUMemory  = Graphics::Renderer::GetGraphicsContext()->GetGPUMemoryUsed();
            stats.TotalGPUMemory = Graphics::Renderer::GetGraphicsContext()->GetTotalGPUMemory();
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

            stats.FramesPerSecond  = m_Frames;
            stats.UpdatesPerSecond = m_Updates;

            m_Frames  = 0;
            m_Updates = 0;
        }

        // Sync transforms from physics for the next frame
        {
            System::JobSystem::Wait(context);

            m_SystemManager->GetSystem<LumosPhysicsEngine>()->SyncTransforms(m_SceneManager->GetCurrentScene());
            m_SystemManager->GetSystem<B2PhysicsEngine>()->SyncTransforms(m_SceneManager->GetCurrentScene());
        }

        if(!m_Minimized)
            OnDebugDraw(); // Moved after update thread sync to fix debug drawing physics Engine

        if(!m_Minimized)
            Graphics::Renderer::GetRenderer()->Present();

        return m_CurrentState != AppState::Closing;
    }

    void Application::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_SceneManager->GetCurrentScene())
            return;

        if(!m_DisableMainSceneRenderer)
        {
            m_SceneRenderer->BeginScene(m_SceneManager->GetCurrentScene());
            m_SceneRenderer->OnRender();
        }
    }

    void Application::OnDebugDraw()
    {
        m_SystemManager->OnDebugDraw();
    }

    void Application::TestUI()
    {
        {
            UIPushStyle(StyleVar_BackgroundColor, { 0.1f, 0.1f, 0.1f, 0.4f });
            UIPushStyle(StyleVar_BorderColor, { 0.4f, 0.4f, 0.4f, 0.6f });
            UIPushStyle(StyleVar_TextColor, { 0.8f, 0.8f, 0.8f, 1.0f });

            // UIPushStyle(StyleVar_FontSize, { 24.0f, 1.0f, 1.0f, 1.0f });
            static bool value                   = false;
            static bool showDebugDearImGuiPanel = false;

            {
                UIBeginPanel("First Panel", WidgetFlags_StackVertically | WidgetFlags_CentreX | WidgetFlags_CentreY);
                if(UIButton("Test Button2").clicked)
                    LINFO("clicked button2");

                UIToggle("Dear ImGui Debug UI Panel", &showDebugDearImGuiPanel);

                UILabel("Info1", Str8Lit("Test 1"));
                UILabel("Info2", Str8Lit("Test 2"));
                UILabel("Info3", Str8Lit("Test 3"));
                UILabel("Info4", Str8Lit("Test 4"));
                UIToggle("Test Toggle", &value);

                if(UIButton("Exit App").clicked)
                    SetAppState(Lumos::AppState::Closing);

                static float valueS = 0.5f;
                UISlider("Test Slider", &valueS);
                UIEndPanel();

                if(showDebugDearImGuiPanel)
                    DearIMGUIDebugPanel();
            }

            if(value)
            {
                UIBeginPanel("Second Panel", WidgetFlags_StackVertically); // WidgetFlags_Draggable |  WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
                UILabel("Info 5", Str8Lit("Test 1234"));
                UILabel("Info 6", Str8Lit("Test 56789"));
                UILabel("Info 7", Str8Lit("Test"));
                UILabel("Info 8", Str8Lit("Test"));
                UILabel("Info 9", Str8Lit("Test"));
                UILabel("Info 1###samenametest", Str8Lit("Test"));
                UIEndPanel();
            }

            UIPopStyle(StyleVar_TextColor);
            UIPopStyle(StyleVar_BorderColor);
            UIPopStyle(StyleVar_BackgroundColor);
            // UIPopStyle(StyleVar_FontSize);
        }
    }

    void Application::OnUpdate(const TimeStep& dt)
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_SceneManager->GetCurrentScene())
            return;

        if(Application::Get().GetEditorState() != EditorState::Paused
           && Application::Get().GetEditorState() != EditorState::Preview)
        {
            LuaManager::Get().OnUpdate(m_SceneManager->GetCurrentScene());
            m_SceneManager->GetCurrentScene()->OnUpdate(dt);
        }
        m_ImGuiManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
    }

    void Application::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        if(m_ImGuiManager)
            m_ImGuiManager->OnEvent(e);
        if(e.Handled())
            return;

        if(m_SceneRenderer)
            m_SceneRenderer->OnEvent(e);

        if(e.Handled())
            return;

        if(m_SceneManager->GetCurrentScene())
            m_SceneManager->GetCurrentScene()->OnEvent(e);

        Input::Get().OnEvent(e);
    }

    void Application::Run()
    {
        while(OnFrame())
        {
        }

        OnQuit();
    }

    void Application::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
        m_SceneRenderer->OnNewScene(scene);
    }

    SharedPtr<AssetManager>& Application::GetAssetManager()
    {
        return m_AssetManager;
    }

    void Application::SubmitToMainThread(const Function<void()>& function)
    {
        LUMOS_PROFILE_FUNCTION();
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

        m_MainThreadQueue.PushBack(function);
    }

    void Application::ExecuteMainThreadQueue()
    {
        LUMOS_PROFILE_FUNCTION();
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);

        for(const auto& func : m_MainThreadQueue)
            func();

        m_MainThreadQueue.Clear();
    }

    void Application::OnExitScene()
    {
    }

    void Application::AddDefaultScene()
    {
        if(m_SceneManager->GetScenes().Size() == 0)
        {
            m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
            m_SceneManager->SwitchScene(0);
        }
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_CurrentState = AppState::Closing;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        int width = e.GetWidth(), height = e.GetHeight();

        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;

        Graphics::Renderer::GetRenderer()->OnResize(width, height);

        if(m_SceneRenderer)
            m_SceneRenderer->OnResize(width, height);

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        return false;
    }

    void Application::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_SceneManager->GetCurrentScene())
            return;

        m_SceneManager->GetCurrentScene()->OnImGui();
    }

    void Application::UpdateSystems()
    {
        LUMOS_PROFILE_FUNCTION();
        if(Application::Get().GetEditorState() != EditorState::Paused
           && Application::Get().GetEditorState() != EditorState::Preview)
        {
            auto scene = Application::Get().GetSceneManager()->GetCurrentScene();

            if(!scene)
                return;

            Application::Get().GetSystemManager()->OnUpdate(Engine::GetTimeStep(), scene);
        }
    }

    void Application::OnSceneViewSizeUpdated(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        WindowResizeEvent e(width, height);
        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return;
        }
        m_Minimized = false;
        m_SceneRenderer->OnResize(width, height);
        m_SceneRenderer->OnEvent(e);

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
    }

    void Application::Serialise()
    {
        LUMOS_PROFILE_FUNCTION();
        {
            std::stringstream storage;
            {
                // output finishes flushing its contents when it goes out of scope
                cereal::JSONOutputArchive output { storage };
                output(*this);
            }
            auto fullPath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");
            LINFO("Serialising Application %s", fullPath.c_str());
            FileSystem::WriteTextFile(fullPath, storage.str());
        }

        // Save Asset Registry
        {
            String8 path = PushStr8F(m_FrameArena, "%sAssetRegistry.lmar", m_ProjectSettings.m_ProjectRoot.c_str()); // m_ProjectSettings.m_ProjectRoot + std::string("AssetRegistry.lmar");
            SerialiseAssetRegistry(path, *m_AssetManager->GetAssetRegistry());
        }
    }

    void Application::Deserialise()
    {
        LUMOS_PROFILE_FUNCTION();
        {
            auto filePath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");

            MountFileSystemPaths();

            LINFO("Loading Project : %s", filePath.c_str());

            if(!FileSystem::FileExists(filePath))
            {
                LINFO("No saved Project file found %s", filePath.c_str());
                {
                    m_SceneManager = CreateUniquePtr<SceneManager>();

                    // Set Default values
                    m_ProjectSettings.RenderAPI   = 1;
                    m_ProjectSettings.Width       = 1200;
                    m_ProjectSettings.Height      = 800;
                    m_ProjectSettings.Borderless  = false;
                    m_ProjectSettings.VSync       = true;
                    m_ProjectSettings.Title       = "App";
                    m_ProjectSettings.ShowConsole = false;
                    m_ProjectSettings.Fullscreen  = false;

                    m_ProjectLoaded = false;

#ifdef LUMOS_PLATFORM_MACOS
                    // This is assuming Application in bin/Release-macos-x86_64/LumosEditor.app
                    LINFO(StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()).c_str());
                    m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../../../Lumos/Assets/";

                    if(!FileSystem::FolderExists(m_ProjectSettings.m_EngineAssetPath))
                    {
                        m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../Lumos/Assets/";
                    }
#else
                    m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../Lumos/Assets/";
#endif
                    m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                    m_SceneManager->SwitchScene(0);
                }
                return;
            }

            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Scripts");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Textures");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Meshes");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Sounds");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs");
            FileSystem::CreateFolderIfDoesntExist(m_ProjectSettings.m_ProjectRoot + "Assets/Materials");

            m_ProjectLoaded = true;

            std::string data = FileSystem::ReadTextFile(filePath);
            std::istringstream istr;
            istr.str(data);
            try
            {
                cereal::JSONInputArchive input(istr);
                input(*this);

                // Load Asset Registry
                {
                    String8 path = PushStr8F(m_FrameArena, "%sAssetRegistry.lmar", m_ProjectSettings.m_ProjectRoot.c_str());
                    if(FileSystem::FileExists((const char*)path.str))
                    {
                        DeserialiseAssetRegistry(path, *m_AssetManager->GetAssetRegistry());
                    }
                }
            }
            catch(...)
            {
                // Set Default values
                m_ProjectSettings.RenderAPI   = 1;
                m_ProjectSettings.Width       = 1200;
                m_ProjectSettings.Height      = 800;
                m_ProjectSettings.Borderless  = false;
                m_ProjectSettings.VSync       = true;
                m_ProjectSettings.Title       = "App";
                m_ProjectSettings.ShowConsole = false;
                m_ProjectSettings.Fullscreen  = false;

#ifdef LUMOS_PLATFORM_MACOS
                m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../../../Lumos/Assets/";
#else
                m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../Lumos/Assets/";
#endif

                m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                m_SceneManager->SwitchScene(0);

                LERROR("Failed to load project - %s", filePath.c_str());
            }
        }
    }
}
