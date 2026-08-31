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
#include "Graphics/PointCloud.h"
#include "Maths/Transform.h"
#include "Maths/MathsUtilities.h"
#include "Scene/EntityFactory.h"
#include "Utilities/LoadImage.h"
#include "Utilities/StringPool.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/Profiler.h"
#include "Core/OS/FileSystem.h"
#include "Core/JobSystem.h"
#include "Core/CoreSystem.h"
#include "Utilities/StringUtilities.h"
#include "Core/String.h"
#include "Core/DataStructures/TDArray.h"
#include "Core/CommandLine.h"
#include "Core/TestRunner.h"
#include "Core/Asset/AssetManager.h"
#include "Core/Asset/AssetImporter.h"
#include "Scripting/Lua/LuaManager.h"
#include "ImGui/ImGuiManager.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
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

#include "Embedded/EmbeddedAssetData.h"
#include "Core/OS/OS.h"
#include "Core/Undo.h"
#include "Core/DebugMenu.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <filesystem>

namespace Lumos
{
    int Application::s_ExitCode = 0;

    // File-static rather than a member: Runtime/Editor subclass Application and
    // the generated projects don't track header deps, so growing the layout
    // silently breaks them.
    static bool s_TestUIRequested = false;

    Application::Application()
        : m_Frames(0)
        , m_Updates(0)
        , m_SceneViewWidth(800)
        , m_SceneViewHeight(600)
        , m_FrameArena(nullptr)
        , m_Arena(nullptr)
        , m_UIArena(nullptr)
        , m_StringPool(nullptr)
    {
    }

    Application::~Application()
    {
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
        m_Arena      = ArenaAlloc(Megabytes(1));

        m_StringPool = new StringPool(m_Arena, 260);

        // Allocate Asset Path String. Set later
        m_AssetPath  = m_StringPool->Allocate("");

        InitialiseUndo();

        m_EventQueueMutex      = PushObject(m_Arena, Mutex);
        m_MainThreadQueueMutex = PushObject(m_Arena, Mutex);

        MutexInit(m_EventQueueMutex);
        MutexSetName(m_EventQueueMutex, "Event Queue");
        MutexInit(m_MainThreadQueueMutex);
        MutexSetName(m_MainThreadQueueMutex, "Main Thread Queue");

        m_EventQueue.Reserve(16);

        SetMaxImageDimensions(2048, 2048);

        m_SceneManager = CreateUniquePtr<SceneManager>();
        m_AssetManager = CreateSharedPtr<AssetManager>();

        Deserialise();

#ifdef LUMOS_PLATFORM_IOS
        m_ProjectSettings.m_EngineAssetPath = OS::Get().GetAssetPath();
#else
        SetEngineAssetPath();
#endif

        bool bDisableSplashScreen = false;
        CommandLine* cmdline      = Internal::CoreSystem::GetCmdLine();
        if(cmdline->OptionBool(Str8Lit("disable-splash")))
        {
            bDisableSplashScreen = true;
        }

        // Check for screenshot option
        String8 screenshotOpt = cmdline->OptionString(Str8Lit("screenshot"));
        if(screenshotOpt.size > 0)
        {
            m_ScreenshotPath = std::string((const char*)screenshotOpt.str, screenshotOpt.size);
            m_TakeScreenshotOnInit = true;
        }

        // Screenshot control
        if(cmdline->OptionBool(Str8Lit("screenshot-no-close")))
        {
            m_CloseAfterScreenshot = false;
        }

        // Frames to wait before capturing — lets screenshots catch mid-game states
        String8 screenshotDelayOpt = cmdline->OptionString(Str8Lit("screenshot-delay"));
        if(screenshotDelayOpt.size > 0)
        {
            int frames = (int)atoi((const char*)screenshotDelayOpt.str);
            if(frames > 0)
                m_ScreenshotFrameTarget = frames;
        }

        // Embedded shaders
        if(cmdline->OptionBool(Str8Lit("force-embedded-shaders")))
        {
            m_ForceEmbeddedShaders = true;
        }

        // Test UI visibility
        if(cmdline->OptionBool(Str8Lit("test-ui")))
        {
            m_ShowTestUI     = true;
            s_TestUIRequested = true;
        }
        if(cmdline->OptionBool(Str8Lit("disable-test-ui")))
        {
            m_ShowTestUI = false;
        }

        // Headless mode
        if(cmdline->OptionBool(Str8Lit("headless")))
        {
            m_HeadlessMode = true;
        }

        // Scene selection
        String8 sceneOpt = cmdline->OptionString(Str8Lit("scene"));
        if(sceneOpt.size > 0)
        {
            m_InitialSceneName = std::string((const char*)sceneOpt.str, sceneOpt.size);
        }

        const bool forceNoVSync = cmdline->OptionBool(Str8Lit("no-vsync"));

        // Benchmark mode
        String8 benchmarkOpt = cmdline->OptionString(Str8Lit("benchmark"));
        if(benchmarkOpt.size > 0)
        {
            m_BenchmarkMode = true;
            m_BenchmarkFrameCount = (int)atoi((const char*)benchmarkOpt.str);
        }

        if(cmdline->OptionBool(Str8Lit("import-assets")))
        {
            m_ImportAssetsMode = true;
        }

        // Scripted test run. The script is a physical path so a stale asset pack
        // can't shadow it the way it can shadow loose //Assets scripts.
        String8 testOpt = cmdline->OptionString(Str8Lit("test"));
        if(testOpt.size > 0)
        {
            TestRunner& runner = TestRunner::Get();
            runner.Init(std::string((const char*)testOpt.str, testOpt.size));
            runner.SetUpdateGolden(cmdline->OptionBool(Str8Lit("test-update-golden")));

            String8 reportOpt = cmdline->OptionString(Str8Lit("test-report"));
            if(reportOpt.size > 0)
                runner.SetReportPath(std::string((const char*)reportOpt.str, reportOpt.size));

            String8 timeoutOpt = cmdline->OptionString(Str8Lit("test-timeout"));
            if(timeoutOpt.size > 0)
            {
                int frames = (int)atoi((const char*)timeoutOpt.str);
                if(frames > 0)
                    runner.SetTimeoutFrames(frames);
            }
        }

        Engine::Get();

        String8 fixedDtOpt = cmdline->OptionString(Str8Lit("fixed-dt"));
        if(fixedDtOpt.size > 0)
        {
            double ms = atof((const char*)fixedDtOpt.str);
            if(ms > 0.0)
                Engine::GetTimeStep().SetFixedTimestep(ms);
        }

        LuaManager::Get().OnInit();
        LuaManager::Get().OnNewProject(m_ProjectSettings.m_ProjectRoot);
        m_Timer = CreateUniquePtr<Timer>();

        Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(m_ProjectSettings.RenderAPI));

        // Window size overrides — handy for previewing portrait/mobile layouts on desktop
        {
            String8 widthOpt  = cmdline->OptionString(Str8Lit("width"));
            String8 heightOpt = cmdline->OptionString(Str8Lit("height"));
            int w = widthOpt.size > 0 ? (int)atoi((const char*)widthOpt.str) : 0;
            int h = heightOpt.size > 0 ? (int)atoi((const char*)heightOpt.str) : 0;
            if(w > 0)
                m_ProjectSettings.Width = (u32)w;
            if(h > 0)
                m_ProjectSettings.Height = (u32)h;
            if(w > 0 || h > 0)
                m_ProjectSettings.Fullscreen = false;
        }

        WindowDesc windowDesc;
        windowDesc.Width       = m_ProjectSettings.Width;
        windowDesc.Height      = m_ProjectSettings.Height;
        windowDesc.RenderAPI   = m_ProjectSettings.RenderAPI;
        windowDesc.Fullscreen  = m_ProjectSettings.Fullscreen;
        windowDesc.Borderless  = m_ProjectSettings.Borderless;
        windowDesc.ShowConsole = m_ProjectSettings.ShowConsole;
        windowDesc.Title       = Str8StdS(m_ProjectSettings.Title);
        windowDesc.VSync       = m_ProjectSettings.VSync && !forceNoVSync;

        if(GetAppType() == AppType::Editor)
        {
            windowDesc.Fullscreen = true;
        }

        if(m_ProjectSettings.DefaultIcon)
        {
            windowDesc.IconPaths = { Str8Lit("//Assets/Textures/icon.png"), Str8Lit("//Assets/Textures/icon32.png") };
        }

        // Initialise the Window (skip in headless mode)
        if(!m_HeadlessMode)
        {
            LUMOS_PROFILE_SCOPE("Init::Window");
            m_Window = UniquePtr<Window>(Window::Create());
            if(!m_Window->Init(windowDesc))
                OnQuit();

            m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

            m_SceneViewWidth  = m_Window->GetWidth();
            m_SceneViewHeight = m_Window->GetHeight();
        }
        else
        {
            m_SceneViewWidth  = windowDesc.Width;
            m_SceneViewHeight = windowDesc.Height;
        }

        m_EditorState = EditorState::Play;

        bool loadEmbeddedShaders = m_ForceEmbeddedShaders;
        if(!m_ForceEmbeddedShaders)
        {
#ifdef LUMOS_PLATFORM_MACOS
            m_ProjectSettings.m_EngineAssetPath = "/Users/jmorton/Dev/Lumos-Dev/Lumos/Assets/";
#else
            m_ProjectSettings.m_EngineAssetPath = "/home/jmorton/Dev/Lumos-Dev/Lumos/Assets/";
#endif
            std::string ShaderFolder = m_ProjectSettings.m_EngineAssetPath + "Shaders";

            if(FileSystem::FolderExists(Str8StdS(ShaderFolder)))
            {
                LINFO("Engine Asset Folder Found");
                loadEmbeddedShaders = false;
            }
            else
            {
                LINFO("Engine Asset Not Folder Found");
                loadEmbeddedShaders = true;
            }
        }

        if(!loadEmbeddedShaders && !m_ForceEmbeddedShaders)
        {
            LUMOS_PROFILE_SCOPE("Init::EmbedShaders");
            ArenaTemp temp     = ScratchBegin(0, 0);
            String8 shaderPath = PushStr8F(temp.arena, "%sShaders/CompiledSPV/", m_ProjectSettings.m_EngineAssetPath.c_str());

            FileSystem::IterateFolder((const char*)shaderPath.str, EmbedShaderFunc);
            ScratchEnd(temp);

            LINFO("Embedded %i shaders.", EmbedShaderCount);
        }
        {
            LUMOS_PROFILE_SCOPE("Init::Renderer");
            Graphics::Renderer::Init(loadEmbeddedShaders, m_ProjectSettings.m_EngineAssetPath);
        }

        if(cmdline->OptionBool(Str8Lit("embed-engine-shaders")))
        {
            EmbedEngineShaders();
        }

        if(m_ProjectSettings.Fullscreen)
        {
#ifndef LUMOS_PLATFORM_MACOS
            m_Window->Maximise();
#endif
        }

        // Draw Splash Screen
        if(!bDisableSplashScreen)
        {
            auto desc          = Graphics::TextureDesc(Graphics::TextureFilter::LINEAR, Graphics::TextureFilter::LINEAR, Graphics::TextureWrap::REPEAT);
            desc.flags         = Graphics::TextureFlags::Texture_Sampled;
            Graphics::Texture2D* splashTexture = nullptr;

            if(!m_ProjectSettings.SplashPath.empty()
               && FileSystem::FileExists(Str8StdS(m_ProjectSettings.SplashPath)))
            {
                splashTexture = Graphics::Texture2D::CreateFromFile("splash", m_ProjectSettings.SplashPath, desc);
            }

            if(!splashTexture)
            {
                const auto& splashData = Embedded::GetSplashData();
                u64 pixelCount   = u64(splashData.width) * u64(splashData.height);
                Arena* splashArena = ArenaAlloc(pixelCount * 4 + 1024);
                u8* composited     = PushArrayNoZero(splashArena, u8, pixelCount * 4);
                const float* bg    = m_ProjectSettings.SplashBGColour;
                for(u64 i = 0; i < pixelCount; i++)
                {
                    const u8* src = splashData.data + i * 4;
                    float a       = src[3] / 255.0f;
                    for(u32 c = 0; c < 3; c++)
                        composited[i * 4 + c] = u8(Maths::Lerp(bg[c] * 255.0f, float(src[c]), a) + 0.5f);
                    composited[i * 4 + 3] = 255;
                }
                splashTexture = Graphics::Texture2D::CreateFromSource(splashData.width, splashData.height, (void*)composited, desc);
                ArenaRelease(splashArena);
            }

            if(splashTexture && Graphics::Renderer::GetRenderer()->Begin())
            {
                Graphics::Renderer::GetRenderer()->DrawSplashScreen(splashTexture, m_ProjectSettings.SplashBGColour);
                Graphics::Renderer::GetRenderer()->Present();
                m_Window->ProcessInput();
                m_Window->OnUpdate();
            }

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

        {
            LUMOS_PROFILE_SCOPE("Init::ImGui");
            m_ImGuiManager = CreateUniquePtr<ImGuiManager>(m_ImGuiClearScreen);
            m_ImGuiManager->OnInit();
            LINFO("Initialised ImGui Manager");
        }

        {
            LUMOS_PROFILE_SCOPE("Init::SceneRenderer");
            m_SceneRenderer = CreateUniquePtr<Graphics::SceneRenderer>(screenWidth, screenHeight);
            LINFO("Initialised SceneRenderer");
        }

        LINFO("Waiting for JobSystem");
        System::JobSystem::Wait(context);

        m_CurrentState = AppState::Running;

        if(m_ImportAssetsMode)
        {
            ImportAllProjectAssets();
            m_BenchmarkMode = true;
            m_BenchmarkFrameCount = 5;
        }

        {
            LUMOS_PROFILE_SCOPE("Init::DefaultTexture");
            Graphics::Material::InitDefaultTexture();
            LINFO("Initialised Default Texture");
        }

        {
            LUMOS_PROFILE_SCOPE("Init::DefaultFont");
            Graphics::Font::InitDefaultFont();
            LINFO("Initialised Default Font");
        }

        m_SceneRenderer->EnableDebugRenderer(true);
        LINFO("Debug Renderer Enabled");

        LuaManager::Get().RunScriptFile(m_ProjectSettings.AppScript);

#ifdef LUMOS_SSE
        LINFO("SSE Maths Enabled");
#endif

        m_UIArena = ArenaAlloc(Megabytes(8));

        InitialiseUI(m_UIArena);
        UIApplyDarkTheme();

        float dpiScale = Application::Get().GetWindow()->GetDPIScale();
        GetUIState()->DPIScale = dpiScale;
        // Mouse mapping scales separately: iOS touches are already native px.
        GetUIState()->InputScale = Application::Get().GetWindow()->GetMousePosScale();

        // Scale UI style defaults by DPI so text/padding are correct on Retina
        if(dpiScale > 1.0f)
        {
            auto* styles = GetUIState()->style_variable_lists;
            styles[StyleVar_FontSize].first->value.x *= dpiScale;
            styles[StyleVar_Padding].first->value.x *= dpiScale;
            styles[StyleVar_Padding].first->value.y *= dpiScale;
            styles[StyleVar_Border].first->value.x *= dpiScale;
            styles[StyleVar_Border].first->value.y *= dpiScale;
            styles[StyleVar_CornerRadius].first->value.x *= dpiScale;
            styles[StyleVar_ShadowOffset].first->value.y *= dpiScale;
            styles[StyleVar_ShadowBlur].first->value.x *= dpiScale;
        }

        // In-game debug overlay (command palette + HUD panels). The editor has
        // its own tooling and its own function-key bindings, so leave it off there.
        DebugMenu::Get().Init();
        DebugMenu::Get().SetEnabled(GetAppType() != AppType::Editor);

        if(cmdline->OptionBool(Str8Lit("test-maths")))
        {
            Maths::TestMaths();
        }

#ifdef LUMOS_PLATFORM_MACOS
        m_QualitySettings.SetGeneralLevel(1);
#else
        m_QualitySettings.SetGeneralLevel(2);
#endif

        if(m_Window)
            m_Window->Show();
    }

    static bool IsImportableModelExt(const std::string& ext)
    {
        return ext == "obj" || ext == "gltf" || ext == "glb" || ext == "fbx" || ext == "FBX";
    }

    void Application::ImportAllProjectAssets()
    {
        LUMOS_PROFILE_FUNCTION();

        const String8& assetsPathStr = FileSystem::Get().GetAssetPath();
        std::string assetsDir((const char*)assetsPathStr.str, assetsPathStr.size);

        int imported = 0, cached = 0, failed = 0;

        try
        {
            for(auto& entry : std::filesystem::recursive_directory_iterator(assetsDir))
            {
                if(!entry.is_regular_file())
                    continue;

                std::string filePath = entry.path().string();
                std::string ext      = StringUtilities::GetFilePathExtension(filePath);
                if(!IsImportableModelExt(ext))
                    continue;

                // Skip files inside Imported/ (the .lmesh cache itself)
                if(filePath.find("/Imported/") != std::string::npos)
                    continue;

                // Build VFS path: strip assetsDir prefix, prepend //Assets/
                std::string rel = filePath.substr(assetsDir.size());
                for(char& c : rel)
                    if(c == '\\') c = '/';
                if(!rel.empty() && rel[0] == '/')
                    rel = rel.substr(1);
                std::string vfsPath = "//Assets/" + rel;

                if(!AssetImporter::NeedsImport(vfsPath))
                {
                    cached++;
                    continue;
                }

                LINFO("[ImportAssets] Importing %s", vfsPath.c_str());
                ImportSettings settings;
                std::string result = AssetImporter::Import(vfsPath, settings);
                if(result.empty())
                {
                    LERROR("[ImportAssets] Failed to import %s", vfsPath.c_str());
                    failed++;
                }
                else
                {
                    imported++;
                }
            }
        }
        catch(const std::exception& e)
        {
            LERROR("[ImportAssets] Filesystem walk failed: %s", e.what());
        }

        LINFO("[ImportAssets] Done: %d imported, %d already cached, %d failed", imported, cached, failed);
    }

    void Application::OnQuit()
    {
        LUMOS_PROFILE_FUNCTION();
        Serialise();

        MutexDestroy(m_EventQueueMutex);
        MutexDestroy(m_MainThreadQueueMutex);

        ReleaseUndo();

        DebugMenu::Get().Shutdown();
        DebugMenu::Release();

        Graphics::ClearPointClouds();

        Graphics::Material::ReleaseDefaultTexture();
        Graphics::Font::ShutdownDefaultFont();
        Engine::Release();
        Input::Release();

        m_SceneManager.reset();
        m_SceneRenderer.reset();
        LuaManager::Release();
        m_SystemManager.reset();
        m_AssetManager.reset();

        Graphics::Pipeline::ClearCache();
        Graphics::RenderPass::ClearCache();
        Graphics::Framebuffer::ClearCache();

        m_ImGuiManager.reset();
        m_Window.reset();

        DebugRenderer::Release();

        Graphics::Renderer::Release();

        ShutDownUI();

        ArenaRelease(m_FrameArena);
        ArenaRelease(m_Arena);

        delete m_StringPool;

        ArenaRelease(m_UIArena);
    }

    void Application::OpenProject(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectSettings.m_ProjectName = StringUtilities::GetFileName(filePath);
        m_ProjectSettings.m_ProjectName = StringUtilities::RemoveFilePathExtension(m_ProjectSettings.m_ProjectName);

        auto projectRoot                = StringUtilities::GetFileLocation(filePath);
        m_ProjectSettings.m_ProjectRoot = projectRoot;

        String8 pathCopy                = PushStr8Copy(m_FrameArena, projectRoot.c_str());
        pathCopy                        = StringUtilities::ResolveRelativePath(m_FrameArena, pathCopy);
        m_ProjectSettings.m_ProjectRoot = (const char*)pathCopy.str;

        CreateAssetFolders();

        Deserialise();

        m_SceneManager->LoadCurrentList();
        m_SceneManager->ApplySceneSwitch();

        // Load specific scene if requested via command line
        if(!m_InitialSceneName.empty())
        {
            auto& scenes = m_SceneManager->GetScenes();
            for(size_t i = 0; i < scenes.Size(); i++)
            {
                if(scenes[i]->GetSceneName() == m_InitialSceneName)
                {
                    m_SceneManager->SwitchScene((int)i);
                    m_SceneManager->ApplySceneSwitch();
                    break;
                }
            }
        }

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

        FileSystem::CreateFolderIfDoesntExist(Str8StdS(m_ProjectSettings.m_ProjectRoot));
        m_SceneManager = CreateUniquePtr<SceneManager>();

        MountFileSystemPaths();

        {
            String8 packPath = PushStr8F(m_FrameArena, "%sAssets.lpak", m_ProjectSettings.m_ProjectRoot.c_str());
            if(FileSystem::FileExists(packPath))
            {
                if(FileSystem::Get().MountPack(packPath))
                    LINFO("Mounted asset pack: %.*s", (int)packPath.size, packPath.str);
            }
        }

        SetEngineAssetPath();
        CreateAssetFolders();

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
        m_AssetPath = Str8F(m_AssetPath, "%sAssets", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::Get().SetAssetPath(m_AssetPath);
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

        auto& ts    = Engine::GetTimeStep();
        auto& stats = Engine::Get().Statistics();
        {
            LUMOS_PROFILE_SCOPE("Application::TimeStepUpdates");
            ts.OnUpdate();

            // Unscaled: a paused game (time scale 0) still has to hand ImGui a
            // positive delta, and debug UI should keep animating while paused.
            ImGuiIO& io  = ImGui::GetIO();
            io.DeltaTime = Maths::Max(0.0001f, (float)(ts.GetUnscaledMillis() * 0.001));

            stats.FrameTime = ts.GetRawMillis(); // Use raw time for stats display
        }

        // Process Input events AFTER time sampling (per Unity's recommendations for lower latency)
        // Input processed here will be used in this frame's Update()
        Input::Get().ResetPressed();
        Input::Get().ResetGestures();
        if(m_Window)
            m_Window->ProcessInput();

        // Scripted input overwrites the real pump, before the UI hit-tests it this frame.
        TestRunner::Get().Step();

        // Every resize the poll above reported collapses into a single rebuild here,
        // before this frame's update or render sees the new size.
        ApplyPendingWindowResize();

        ArenaClear(m_FrameArena);

#ifdef LUMOS_PLATFORM_MACOS
        const bool wantFullscreen = m_ProjectSettings.Fullscreen || GetAppType() == AppType::Editor;
        // 0 = toggle pending, 1 = confirming it entered, 2 = done.
        static int s_FullscreenStage    = 0;
        static int s_FullscreenFrame    = 0;
        static int s_FullscreenAttempts = 0;
        if(wantFullscreen && s_FullscreenStage < 2 && m_Window)
        {
            s_FullscreenFrame++;
            if(s_FullscreenStage == 0 && s_FullscreenFrame >= 2)
            {
                OS::Get().SetWindowFullscreen(true);
                s_FullscreenStage    = 1;
                s_FullscreenFrame    = 0;
                s_FullscreenAttempts = 1;
            }
            else if(s_FullscreenStage == 1)
            {
                if(OS::Get().DidEnterFullscreen())
                {
                    s_FullscreenStage = 2;
                }
                else if(s_FullscreenFrame >= 150)
                {
                    if(s_FullscreenAttempts < 8)
                    {
                        OS::Get().SetWindowFullscreen(true);
                        s_FullscreenAttempts++;
                        s_FullscreenFrame = 0;
                    }
                    else
                    {
                        s_FullscreenStage = 2; // give up rather than loop forever
                    }
                }
            }
        }
#endif

        if(m_SceneManager->GetSwitchingScene())
        {
            LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
            Graphics::Renderer::GetGraphicsContext()->WaitIdle();
            m_SceneManager->ApplySceneSwitch();
            return m_CurrentState != AppState::Closing;
        }

        double now = m_Timer->GetElapsedSD();

        static int s_NumContiguousLargeFrames = 0;
        const int maxContiguousLargeFrames    = 2;

        if(ts.GetRawSeconds() > 5)
        {
            LWARN("Large frame time %.2f", ts.GetRawSeconds());

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
            LUMOS_PROFILE_SCOPE("ImGui::NewFrame");
            ImGui::NewFrame();
        }

        // The UI runs on unscaled time so the overlay still animates when the
        // game is paused or slowed down.
        const float uiDelta = Maths::Max(0.0001f, (float)(ts.GetUnscaledMillis() * 0.001));

        Vec2 frameSize = { (float)m_SceneViewWidth, (float)m_SceneViewHeight };
        UIBeginFrame(frameSize, uiDelta, m_SceneViewPosition);
        UIBeginBuild();

        // Before OnUpdate so the palette can take the keyboard off the game.
        DebugMenu::Get().OnUpdate(uiDelta);

        // Escape only toggles the demo panel for runs that asked for it - games
        // use Escape for their own menus and shouldn't get a debug panel on top.
        if(s_TestUIRequested && m_Window && Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape))
            m_ShowTestUI = !m_ShowTestUI;
        if(m_ShowTestUI)
            TestUI();

        {
            ScopedMutex lock(m_EventQueueMutex);

            for(auto& event : m_EventQueue)
            {
                event();
            }
            m_EventQueue.Clear();
        }

        System::JobSystem::Context context;
        if(!m_Minimized)
        {
            if(!Graphics::Renderer::GetRenderer()->Begin())
            {
                ImGui::Render();
                return m_CurrentState != AppState::Closing;
            }
        }
        m_ImGuiManager->OnNewFrame();

#ifdef LUMOS_PLATFORM_IOS
        {
            const bool isEditor = GetAppType() == AppType::Editor;
            SafeAreaInsets sa = UIResolveSafeAreaInsets(OS::Get().GetSafeAreaInsets(), isEditor);
            ImGuiViewportP* vp = (ImGuiViewportP*)ImGui::GetMainViewport();

            const ImVec2 vpPos  = vp->Pos;
            const ImVec2 vpSize = vp->Size;

            if(isEditor && (sa.left > 0.0f || sa.right > 0.0f || sa.top > 0.0f || sa.bottom > 0.0f))
            {
                const ImU32 bg = ImGui::GetColorU32(ImGuiCol_DockingEmptyBg);
                ImDrawList* dl = ImGui::GetBackgroundDrawList(vp);
                if(sa.top > 0.0f)    dl->AddRectFilled(vpPos, ImVec2(vpPos.x + vpSize.x, vpPos.y + sa.top), bg);
                if(sa.bottom > 0.0f) dl->AddRectFilled(ImVec2(vpPos.x, vpPos.y + vpSize.y - sa.bottom), ImVec2(vpPos.x + vpSize.x, vpPos.y + vpSize.y), bg);
                if(sa.left > 0.0f)   dl->AddRectFilled(ImVec2(vpPos.x, vpPos.y + sa.top), ImVec2(vpPos.x + sa.left, vpPos.y + vpSize.y - sa.bottom), bg);
                if(sa.right > 0.0f)  dl->AddRectFilled(ImVec2(vpPos.x + vpSize.x - sa.right, vpPos.y + sa.top), ImVec2(vpPos.x + vpSize.x, vpPos.y + vpSize.y - sa.bottom), bg);
            }

            vp->BuildWorkOffsetMin = ImVec2(sa.left, sa.top);
            vp->BuildWorkOffsetMax = ImVec2(-sa.right, -sa.bottom);

            ImVec2 workMinSeed = vp->WorkOffsetMin;
            ImVec2 workMaxSeed = vp->WorkOffsetMax;
            if(workMinSeed.x < sa.left)  workMinSeed.x = sa.left;
            if(workMinSeed.y < sa.top)   workMinSeed.y = sa.top;
            if(workMaxSeed.x > -sa.right)  workMaxSeed.x = -sa.right;
            if(workMaxSeed.y > -sa.bottom) workMaxSeed.y = -sa.bottom;
            vp->WorkOffsetMin = workMinSeed;
            vp->WorkOffsetMax = workMaxSeed;
            vp->UpdateWorkRect();
        }
#endif

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

        // After the game update so the fly camera and gizmo win over game code,
        // and before rendering so their transforms land this frame.
        DebugMenu::Get().OnPostUpdate(uiDelta);

        // Built last so the overlay sits on top of whatever the game drew.
        DebugMenu::Get().OnUI();

        UIEndBuild();
        UILayout();
        UIAnimate();

        UIEndFrame(Graphics::Font::GetDefaultFont());

        if(!m_Minimized)
        {
            LUMOS_PROFILE_SCOPE("Application::Render");
            Engine::Get().ResetStats();

            OnRender();
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
            if(m_Window)
            {
                m_Window->UpdateCursorImGui();
                m_Window->OnUpdate();
            }
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

        // Test-requested captures - the swap chain image is only readable here.
        TestRunner::Get().ServiceScreenshots(!m_Minimized && m_SceneRenderer && m_Window);

        // Take screenshot after a few frames so UI is stable
        if(m_TakeScreenshotOnInit && !m_ScreenshotTaken && m_CurrentState == AppState::Running)
        {
            m_ScreenshotFrameDelay++;
            if(m_ScreenshotFrameDelay >= m_ScreenshotFrameTarget)
            {
                m_ScreenshotTaken = true;
                if(!m_ScreenshotPath.empty() && m_SceneRenderer && m_Window)
                {
                    LINFO("Taking screenshot: %s", m_ScreenshotPath.c_str());
                    Graphics::Renderer::GetRenderer()->SaveScreenshot(m_ScreenshotPath, Application::Get().GetWindow()->GetSwapChain()->GetCurrentImage());

                    if(m_CloseAfterScreenshot)
                    {
                        m_CurrentState = AppState::Closing;
                    }
                }
            }
        }

        // Benchmark mode handling
        if(m_BenchmarkMode && m_CurrentState == AppState::Running)
        {
            m_BenchmarkCurrentFrame++;
            if(m_BenchmarkCurrentFrame >= m_BenchmarkFrameCount)
            {
                LINFO("Benchmark complete: %d frames", m_BenchmarkFrameCount);
                LINFO("Average FPS: %.2f", stats.FramesPerSecond);
                LINFO("Average Frame Time: %.2fms", stats.FrameTime);
                m_CurrentState = AppState::Closing;
            }
        }

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

    void Application::ExitApp()
    {
        SetAppState(Lumos::AppState::Closing);
    }

    void Application::TestUI()
    {
        {
            // UIPushStyle(StyleVar_BackgroundColor, { 0.1f, 0.1f, 0.1f, 0.4f });
            // UIPushStyle(StyleVar_BorderColor, { 0.4f, 0.4f, 0.4f, 0.6f });
            // UIPushStyle(StyleVar_TextColor, { 0.8f, 0.8f, 0.8f, 1.0f });

            static bool showSecondPanel         = false;
            static bool showDebugDearImGuiPanel = false;
            static bool checkboxValue           = true;
            static int currentTheme             = (int)UITheme_Dark;
            static float progressValue          = 0.0f;
            static int intSliderValue           = 50;
            static float floatSliderValue       = 0.5f;
            static bool expanderOpen            = false;
            static bool nestedExpanderOpen      = false;
            static char textBuffer[64]          = "Hello";
            static char textBuffer2[64]         = "World";
            static int dropdownSelection        = 0;
            static const char* dropdownOptions[] = { "Option A", "Option B", "Option C" };
            static bool showModal               = false;
            static int activeTab                = 0;
            static bool treeNode1               = false;
            static bool treeNode2               = false;
            static bool treeNode1_1             = false;
            static float colorRGB[3]            = { 1.0f, 0.5f, 0.2f };
            static float uiFontSize             = GetUIState()->style_variable_lists[StyleVar_FontSize].first->value.x;
            static float scrollOffset           = 0.0f;
            static int dockMode                 = -1; // -1 = floating

            {
                if(dockMode >= 0)
                {
                    UIBeginPanel("UI Demo",
                        WidgetFlags_Draggable | WidgetFlags_StackVertically | WidgetFlags_DragParent | WidgetFlags_CentreX);
                    UIWindowDock((UIDockPosition)dockMode, 0.4f);
                }
                else
                {
                    UIBeginPanel("UI Demo",
                         WidgetFlags_StackVertically | WidgetFlags_CentreX | WidgetFlags_CentreY | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_DragParent);
                }

                // Theme selector
                static const char* themeNames[] = { "Light", "Dark", "Blue", "Green", "Purple", "High Contrast" };
                if(UIDropdown("Theme", &currentTheme, themeNames, (int)UITheme_Count).clicked)
                {
                    UISetTheme((UITheme)currentTheme);
                    uiFontSize = GetUIState()->style_variable_lists[StyleVar_FontSize].first->value.x;
                }

                {
                    auto result = UISlider("Font Size", &uiFontSize, 12.0f, 64.0f, 200.0f);
                    if(result.dragging || result.clicked)
                        GetUIState()->style_variable_lists[StyleVar_FontSize].first->value.x = uiFontSize;
                }

                // Dock demo
                UIBeginRow();
                if(UIButton("Dock L").clicked) dockMode = (int)Dock_Left;
                if(UIButton("Dock R").clicked) dockMode = (int)Dock_Right;
                if(UIButton("Fill").clicked)   dockMode = (int)Dock_Fill;
                if(UIButton("Float").clicked)  dockMode = -1;
                UIEndRow();

                UISeparator();

                UIBeginRow();
                if(UIButton("Btn A").clicked) LINFO("A");
                if(UIButton("Btn B").clicked) LINFO("B");
                if(UIButton("Btn C").clicked) LINFO("C");
                UIEndRow();

                UISeparator();

                progressValue += 0.003f;
                if(progressValue > 1.0f) progressValue = 0.0f;
                UIProgressBar("Progress", progressValue, 200.0f, 20.0f);

                UISpacer(5.0f);

                UISlider("Volume", &floatSliderValue);
                UISliderInt("Count", &intSliderValue, 0, 100);

                UISeparator();

                // Expander demo
                UIExpander("More Options", &expanderOpen);
                if(expanderOpen)
                {
                    UIBeginExpanderContent("More Options");
                    UITextInput("Name", textBuffer, sizeof(textBuffer));
                    UITextInput("Other", textBuffer2, sizeof(textBuffer2));
                    UIDropdown("Selection", &dropdownSelection, dropdownOptions, 3);
                    UIToggle("Option 1", &showSecondPanel);
                    UIToggle("Option 2", &showDebugDearImGuiPanel);
                    UICheckbox("Enable Feature", &checkboxValue);

                    // Nested expander test
                    UIExpander("Advanced", &nestedExpanderOpen);
                    if(nestedExpanderOpen)
                    {
                        UIBeginExpanderContent("Advanced");
                        UIColorEdit3("Color", colorRGB);
                        UILabel("NestedInfo", Str8Lit("Nested expander content"));
                        UIEndExpanderContent();
                    }

                    // Tree view
                    if(UITreeNode("Root Node", &treeNode1))
                    {
                        if(UITreeNode("Child 1", &treeNode1_1))
                        {
                            UILabel("Leaf1", Str8Lit("Leaf Item A"));
                            UILabel("Leaf2", Str8Lit("Leaf Item B"));
                            UITreePop();
                        }
                        UILabel("Child2Label", Str8Lit("Child 2 (no children)"));
                        UITreePop();
                    }
                    if(UITreeNode("Another Root", &treeNode2))
                    {
                        UILabel("AnotherLeaf", Str8Lit("Another leaf"));
                        UITreePop();
                    }
                    UIEndExpanderContent();
                }

                UIButton("Hover Me");
                UITooltip("This is a tooltip!");

                // Tab demo
                UISeparator();
                static int selectedTab = 0;
                UIBeginTabBar("DemoTabs");
                if(UITabItem("Tab 1")) selectedTab = 0;
                if(UITabItem("Tab 2")) selectedTab = 1;
                if(UITabItem("Tab 3")) selectedTab = 2;
                UIEndTabBar();

                // Tab content (after tab bar)
                if(selectedTab == 0)
                    UILabel("Tab1Content", Str8Lit("Content for Tab 1"));
                else if(selectedTab == 1)
                    UILabel("Tab2Content", Str8Lit("Content for Tab 2"));
                else
                    UILabel("Tab3Content", Str8Lit("Content for Tab 3"));

                UISeparator();

                // Modal demo
                if(UIButton("Open Modal").clicked)
                    showModal = true;

                // Context menu trigger
                if(UIButton("Right-Click Me").right_clicked)
                    GetUIState()->ContextMenuOpen = true;

                UISeparator();

                // Text wrap / ellipsis demo
                UILabelWrapped("WrapDemo", Str8Lit("This is a long sentence that word-wraps to the width given, and hard-breaks superlongunbrokenwords too."), 220.0f);
                UILabelEllipsis("EllipsisDemo", Str8Lit("This long line gets truncated with an ellipsis"), 180.0f);

                // Flex-grow demo: fixed | grow(1) | grow(2)
                {
                    UIBeginRow();
                    UI_Widget* row = GetUIState()->parents.Back();
                    row->semantic_size[UIAxis_X] = { SizeKind_PercentOfParent, 1.0f };
                    UIColouredBox("GrowFixed", 40.0f, 14.0f, Vec4(0.8f, 0.3f, 0.3f, 1.0f), 3.0f);
                    {
                        UI_Interaction g1 = UIColouredBox("Grow1", 1.0f, 14.0f, Vec4(0.3f, 0.8f, 0.3f, 1.0f), 3.0f);
                        g1.widget->semantic_size[UIAxis_X] = { SizeKind_Grow, 1.0f };
                        UI_Interaction g2 = UIColouredBox("Grow2", 1.0f, 14.0f, Vec4(0.3f, 0.3f, 0.8f, 1.0f), 3.0f);
                        g2.widget->semantic_size[UIAxis_X] = { SizeKind_Grow, 2.0f };
                    }
                    UIEndRow();
                }

                // Exit-animation demo: panel below fades out when hidden
                static bool showFadePanel = false;
                if(UIButton(showFadePanel ? "Hide Fading Panel" : "Show Fading Panel").clicked)
                    showFadePanel = !showFadePanel;

                UISeparator();

                if(UIButton("Exit").clicked)
                    SetAppState(Lumos::AppState::Closing);

                UIEndPanel();

                if(showFadePanel)
                {
                    UIBeginPanel("Fading Panel", WidgetFlags_StackVertically | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_DragParent | WidgetFlags_AnimateExit | WidgetFlags_AnimateAppear);
                    UIWindowAnchor(UIAnchor_BottomCenter, 0.0f, 40.0f);
                    UILabel("FadeL1", Str8Lit("This panel fades out when hidden"));
                    UILabel("FadeL2", Str8Lit("(AnimateExit on the panel covers the subtree)"));
                    UIEndPanel();
                }

                // Context menu
                if(UIBeginContextMenu("DemoContextMenu"))
                {
                    if(UIContextMenuItem("Action 1").clicked) LINFO("Action 1");
                    if(UIContextMenuItem("Action 2").clicked) LINFO("Action 2");
                    if(UIContextMenuItem("Action 3").clicked) LINFO("Action 3");
                    UIEndContextMenu();
                }

                // Modal dialog
                if(UIBeginModal("Demo Modal", &showModal))
                {
                    UILabel("ModalTitle", Str8Lit("Modal Dialog"));
                    UISeparator();
                    UILabel("ModalContent", Str8Lit("This is a modal dialog."));
                    UILabel("ModalContent2", Str8Lit("Click outside or OK to close."));
                    UISpacer(10.0f);
                    if(UIButton("OK").clicked)
                        showModal = false;
                    UIEndModal();
                }

                if(showDebugDearImGuiPanel)
                    DearIMGUIDebugPanel();
            }

            // StyleVar_Alpha ladder: four identical panels at descending opacity.
            // A pushed alpha applies to every widget built inside the push, so
            // each panel and its contents should step visibly darker/fainter.
            // Screenshot this to check the whole opacity path in one run.
            {
                const float alphas[] = { 1.0f, 0.6f, 0.3f, 0.1f };
                const char* names[]  = { "Alpha 1.0", "Alpha 0.6", "Alpha 0.3", "Alpha 0.1" };
                for(int a = 0; a < 4; a++)
                {
                    UIPushStyle(StyleVar_Alpha, alphas[a]);
                    UIBeginPanel(names[a], WidgetFlags_StackVertically | WidgetFlags_Floating_X | WidgetFlags_Floating_Y);
                    UIWindowAnchor(UIAnchor_TopRight, 20.0f, 20.0f + (float)a * 110.0f);
                    UILabelCStr("alphaladderlabel", "Opacity test");
                    UIColouredBox(names[a], 120.0f, 18.0f, Vec4(0.35f, 0.75f, 0.95f, 1.0f), 3.0f);
                    UIEndPanel();
                    UIPopStyle(StyleVar_Alpha);
                }
            }

            if(showSecondPanel)
            {
                UIBeginPanel("Info", WidgetFlags_StackVertically | WidgetFlags_Draggable | WidgetFlags_Floating_X | WidgetFlags_Floating_Y | WidgetFlags_DragParent);
                UILabel("L1", Str8Lit("Immediate Mode UI"));
                UILabel("L2", Str8Lit("Custom Widgets"));
                UISeparator();
                UILabel("DblClick", Str8Lit("Double-click test:"));
                if(UIButton("Double Click Me").double_clicked)
                    LINFO("Double clicked!");
                UIEndPanel();
            }

            //UIPopStyle(StyleVar_TextColor);
            //UIPopStyle(StyleVar_BorderColor);
            //UIPopStyle(StyleVar_BackgroundColor);
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

        // Resizes arrive in bursts: the window manager sends one configure event
        // per step of an interactive edge drag, and every one of them would
        // rebuild the swapchain and each render target. That stalls the frame far
        // longer than the drag takes, so the window ends up showing whatever was
        // last presented - old borders left behind at the sizes the drag passed
        // through. Keep only the newest size and apply it once, at the top of the
        // next frame.
        if(e.GetEventType() == WindowResizeEvent::GetStaticType())
        {
            WindowResizeEvent& resizeEvent = static_cast<WindowResizeEvent&>(e);
            m_PendingWindowWidth           = resizeEvent.GetWidth();
            m_PendingWindowHeight          = resizeEvent.GetHeight();
            m_PendingWindowDPIScale        = resizeEvent.GetDPIScale();
            m_HasPendingWindowResize       = true;
            return;
        }

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

        // UI keyboard input
        dispatcher.Dispatch<KeyTypedEvent>([](KeyTypedEvent& event) -> bool
        {
            UIProcessKeyTyped(event.Character);
            return false;
        });
        dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& event) -> bool
        {
            UIProcessKeyPressed(event.GetKeyCode());
            return false;
        });

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
        ScopedMutex lock(m_MainThreadQueueMutex);

        m_MainThreadQueue.PushBack(function);
    }

    void Application::ExecuteMainThreadQueue()
    {
        LUMOS_PROFILE_FUNCTION();
        ScopedMutex lock(m_MainThreadQueueMutex);

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

        m_SceneViewWidth  = width;
        m_SceneViewHeight = height;

        if(m_SceneRenderer)
            m_SceneRenderer->OnResize(width, height);

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        return false;
    }

    void Application::ApplyPendingWindowResize()
    {
        LUMOS_PROFILE_FUNCTION();
        if(!m_HasPendingWindowResize)
            return;

        m_HasPendingWindowResize = false;

        const uint32_t width  = m_PendingWindowWidth;
        const uint32_t height = m_PendingWindowHeight;

        // A zero size still has to reach OnWindowResize to flag minimised, but a
        // repeat of the size already in use is pure rebuild cost.
        if(width != 0 && height != 0 && width == m_AppliedWindowWidth && height == m_AppliedWindowHeight)
            return;

        m_AppliedWindowWidth  = width;
        m_AppliedWindowHeight = height;

        WindowResizeEvent e(width, height, m_PendingWindowDPIScale);

        EventDispatcher dispatcher(e);
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

    // Temp - Copied
    template <class Archive>
    std::string save_minimal(Archive& ar, String8 const& str)
    {
        return ToStdString(str);
    }

    template <class Archive>
    void load_minimal(Archive& ar, String8& str, const std::string& value)
    {
        str = PushStr8Copy(Application::Get().GetFrameArena(), value.c_str());
    }

    void Application::Serialise()
    {
        LUMOS_PROFILE_FUNCTION();

        // A test run must leave the project exactly as it found it - otherwise
        // its --width/--height end up committed to the .lmproj.
        if(TestRunner::Get().Active())
        {
            LINFO("Test run - skipping project serialisation");
            return;
        }

        {
            std::stringstream storage;
            {
                // output finishes flushing its contents when it goes out of scope
                cereal::JSONOutputArchive output { storage };
                output(*this);
            }
            auto fullPath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");
            LINFO("Serialising Application %s", fullPath.c_str());
            FileSystem::WriteTextFile(Str8StdS(fullPath), Str8StdS(storage.str()));
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
            std::string filePath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");

            MountFileSystemPaths();

            {
                String8 packPath = PushStr8F(m_FrameArena, "%sAssets.lpak", m_ProjectSettings.m_ProjectRoot.c_str());
                if(FileSystem::FileExists(packPath))
                {
                    if(FileSystem::Get().MountPack(packPath))
                        LINFO("Mounted asset pack: %.*s", (int)packPath.size, packPath.str);
                }
            }

            LINFO("Loading Project : %s", filePath.c_str());

            m_SceneManager = CreateUniquePtr<SceneManager>();

            if(!FileSystem::FileExists(Str8StdS(filePath)))
            {
                LINFO("No saved Project file found %s", filePath.c_str());
                {
                    // Set Default values
                    m_ProjectSettings = {};
                    m_ProjectLoaded   = false;

                    SetEngineAssetPath();
                    m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                    m_SceneManager->SwitchScene(0);
                }
                return;
            }

            CreateAssetFolders();

            String8 data = FileSystem::ReadTextFile(m_FrameArena, Str8StdS(filePath));

            if(data.size == 0 || data.str == nullptr)
            {
                LERROR("Failed to read project file - %s (data.size=%llu, str=%p)", filePath.c_str(), data.size, (void*)data.str);
                m_ProjectLoaded = false;
                m_ProjectSettings = {};
                SetEngineAssetPath();
                m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                m_SceneManager->SwitchScene(0);
                return;
            }

            NullTerminate(data);

            std::istringstream istr;
            istr.str((const char*)data.str);
            try
            {
                cereal::JSONInputArchive input(istr);
                input(*this);

                // Clear project assets but keep engine assets (shaders etc)
                {
                    m_AssetManager->GetAssetRegistry()->ClearProjectAssets();
                    String8 path = PushStr8F(m_FrameArena, "%sAssetRegistry.lmar", m_ProjectSettings.m_ProjectRoot.c_str());
                    if(FileSystem::FileExists(path))
                    {
                        DeserialiseAssetRegistry(path, *m_AssetManager->GetAssetRegistry());
                    }
                }

                m_ProjectLoaded = true;
            }
            catch(const std::exception& e)
            {
                m_ProjectLoaded   = false;
                m_ProjectSettings = {};
                SetEngineAssetPath();

                m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                m_SceneManager->SwitchScene(0);

                LERROR("Failed to load project - %s", filePath.c_str());
                LERROR("Error: %s", e.what());
            }
        }
    }

    void Application::CreateAssetFolders()
    {
        ArenaTemp tempArena = ScratchBegin(nullptr, 0);
        String8 path        = PushStr8F(tempArena.arena, "%sAssets", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Scripts", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Scenes", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Textures", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Meshes", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Sounds", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Prefabs", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ArenaClear(tempArena.arena);

        path = PushStr8F(tempArena.arena, "%sAssets/Materials", m_ProjectSettings.m_ProjectRoot.c_str());
        FileSystem::CreateFolderIfDoesntExist(path);
        ScratchEnd(tempArena);
    }

    void Application::SetEngineAssetPath()
    {
#ifdef LUMOS_PLATFORM_MACOS
        std::string execPath = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath());
        LINFO(execPath.c_str());

        // Try bundle Resources path first (sandboxed/distributed app)
        std::string bundlePath = execPath + "../Resources/EngineAssets/";
        if(FileSystem::FolderExists(Str8StdS(bundlePath)))
        {
            m_ProjectSettings.m_EngineAssetPath = bundlePath;
            return;
        }

        // Development: relative to build output
        m_ProjectSettings.m_EngineAssetPath = execPath + "../../../../../Lumos/Assets/";
        if(!FileSystem::FolderExists(Str8StdS(m_ProjectSettings.m_EngineAssetPath)))
            m_ProjectSettings.m_EngineAssetPath = execPath + "../../Lumos/Assets/";
#else
        m_ProjectSettings.m_EngineAssetPath = StringUtilities::GetFileLocation(OS::Get().GetExecutablePath()) + "../../Lumos/Assets/";
#endif
    }

    bool Application::EmbedEngineShaders()
    {
        std::string coreDataPath;
        auto shaderPath = std::filesystem::path(m_ProjectSettings.m_EngineAssetPath + "Shaders/CompiledSPV/");
        int shaderCount = 0;
        if(std::filesystem::is_directory(shaderPath))
        {
            for(auto entry : std::filesystem::directory_iterator(shaderPath))
            {
                auto extension = StringUtilities::GetFilePathExtension(entry.path().string());
                if(extension == "spv")
                {
                    EmbedShader(entry.path().string());
                    shaderCount++;
                }
            }
        }
        LINFO("Embedded %i shaders. Recompile to use", shaderCount);
        return shaderCount > 0;
    }
}
