#include "Precompiled.h"
#include "Utilities/AsyncImageLoader.h"
#include "Audio/AudioTagReader.h"
#include "Utilities/AsyncFileCopy.h"
#include "Utilities/AsyncFileList.h"
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
#include "Scene/SceneUndo.h"
#include "Core/BusyOverlay.h"
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

#if !defined(LUMOS_PLATFORM_WINDOWS)
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

namespace Lumos
{
    namespace Graphics
    {
        void InstallOzzAllocator(); // AnimationController.cpp
    }

    static bool s_ImGuiEnabled = true;

    void Application::SetImGuiEnabled(bool enabled) { s_ImGuiEnabled = enabled; }
    bool Application::IsImGuiEnabled() { return s_ImGuiEnabled; }

    int Application::s_ExitCode = 0;

    // File-static rather than a member: Runtime/Editor subclass Application and
    // the generated projects don't track header deps, so growing the layout
    // silently breaks them.
    static bool s_TestUIRequested = false;
    static std::thread::id s_MainThreadId;

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
#if !defined(LUMOS_PLATFORM_WINDOWS)
    // A fatal signal prints where it happened to stderr (a phone has no debugger attached to ask)
    static void CrashHandler(int sig)
    {
        void* frames[64];
        const int count = backtrace(frames, 64);
        const char msg[] = "\n[CRASH] fatal signal, backtrace:\n";
        write(2, msg, sizeof(msg) - 1);
        backtrace_symbols_fd(frames, count, 2);
        signal(sig, SIG_DFL);
        raise(sig);
    }

    static void InstallCrashHandler()
    {
        for(int sig : { SIGSEGV, SIGBUS, SIGABRT, SIGILL, SIGFPE })
            signal(sig, CrashHandler);
    }
#else
    static void InstallCrashHandler() {}
#endif

    void Application::Init()
    {
        Graphics::InstallOzzAllocator();
        InstallCrashHandler();
        LUMOS_PROFILE_FUNCTION();
        m_FrameArena = ArenaAlloc(Megabytes(1));
        m_Arena      = ArenaAlloc(Megabytes(1));

        m_StringPool = new StringPool(m_Arena, 260);

        // Allocate Asset Path String. Set later
        m_AssetPath  = m_StringPool->Allocate("");

        InitialiseUndo();
        SceneUndo::Init();

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

        // runs that need a known window leave the remembered one alone
        if(m_BenchmarkMode || m_TakeScreenshotOnInit || m_HeadlessMode || cmdline->OptionString(Str8Lit("test")).size > 0
           || cmdline->OptionBool(Str8Lit("no-window-memory")))
            m_RememberWindow = false;
        if(cmdline->OptionBool(Str8Lit("window-memory")))
            m_RememberWindow = true; // tests of the feature itself

        if(m_BenchmarkMode || m_TakeScreenshotOnInit
           || cmdline->OptionString(Str8Lit("test")).size > 0
           || cmdline->OptionBool(Str8Lit("no-app-nap")))
        {
            OS::Get().SetLowLatencyMode(true);
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
            RestoreWindowState();

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
            // the path worked out at start-up (a shipped app's own Resources) wins; the dev checkout
            // is only a fallback - a sandboxed app can see it exists but not read it
#ifdef LUMOS_PLATFORM_IOS
            m_ProjectSettings.m_EngineAssetPath = "/home/jmorton/Dev/Lumos-Dev/Lumos/Assets/"; // not there: iOS uses the embedded shaders
#else
            std::string computedShaders = m_ProjectSettings.m_EngineAssetPath + "Shaders";
            if(!FileSystem::FolderExists(Str8StdS(computedShaders)))
            {
#ifdef LUMOS_PLATFORM_MACOS
                m_ProjectSettings.m_EngineAssetPath = "/Users/jmorton/Dev/Lumos-Dev/Lumos/Assets/";
#else
                m_ProjectSettings.m_EngineAssetPath = "/home/jmorton/Dev/Lumos-Dev/Lumos/Assets/";
#endif
            }
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

        // regenerating the embedded shader headers is for dev checkouts; a shipped bundle is read-only
        const bool shippedBundle = m_ProjectSettings.m_EngineAssetPath.find("Resources/EngineAssets") != std::string::npos;
        if(!loadEmbeddedShaders && !m_ForceEmbeddedShaders && !shippedBundle)
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

            // kept: the frames drawn while the first scene is on its way show the same picture
            m_SplashTexture = splashTexture;
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

        // games keep ImGui (debug panels, the Lua bindings); the editor draws everything on the engine UI
        if(s_ImGuiEnabled)
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

        s_MainThreadId = std::this_thread::get_id();

        m_UIArena = ArenaAlloc(Megabytes(8));

        InitialiseUI(m_UIArena);
        UIApplyDarkTheme();

        // Content scale, not DPIScale: the two are the same on Retina/iOS, but on
        // X11 DPIScale is 1.0 even at 200% desktop scaling and the whole UI would
        // be laid out at 1x on a 4K panel. See Window::GetContentScale.
        float dpiScale         = Application::Get().GetWindow()->GetContentScale();
        GetUIState()->DPIScale = dpiScale;
        // Mouse mapping scales separately: iOS touches are already native px, and
        // X11 mouse coordinates are already framebuffer pixels.
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

        {
            // --quality=0|1|2 or LUMOS_QUALITY forces a level; otherwise the GPU decides
            int forced              = -1;
            String8 qualityOpt      = cmdline->OptionString(Str8Lit("quality"));
            if(qualityOpt.size > 0)
                forced = atoi((const char*)qualityOpt.str);
            else if(const char* env = getenv("LUMOS_QUALITY"))
                forced = atoi(env);

            if(GetAppType() == AppType::Editor)
            {
                // the editor keeps full quality at native resolution
#ifdef LUMOS_PLATFORM_MACOS
                m_QualitySettings.SetGeneralLevel(1);
#else
                m_QualitySettings.SetGeneralLevel(2);
#endif
            }
            else
            {
                SetQualityLevel(forced);
                m_QualitySettings.DynamicResolution = forced < 0;
            }
        }

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
                std::string result = AssetImporter::Import(vfsPath);
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

    // <user data>/<title>/window.cfg: "x y w h maximised fullscreen"
    static std::string WindowStatePath(const std::string& title)
    {
        std::string base;
#if defined(LUMOS_PLATFORM_WINDOWS)
        if(const char* appData = std::getenv("APPDATA"))
            base = appData;
#elif defined(LUMOS_PLATFORM_MACOS)
        if(const char* home = std::getenv("HOME"))
            base = std::string(home) + "/Library/Application Support";
#else
        if(const char* xdg = std::getenv("XDG_DATA_HOME"))
            base = xdg;
        else if(const char* home = std::getenv("HOME"))
            base = std::string(home) + "/.local/share";
#endif
        if(base.empty())
            return {};
        std::string name = title.empty() ? "Lumos" : title;
        for(char& c : name)
            if(c == '/' || c == '\\' || c == ':')
                c = '_';
        return base + "/" + name + "/window.cfg";
    }

    void Application::SaveWindowState()
    {
#if !defined(LUMOS_PLATFORM_IOS) && !defined(LUMOS_PLATFORM_ANDROID)
        if(!m_RememberWindow || !m_Window)
            return;
        std::string path = WindowStatePath(m_ProjectSettings.Title);
        if(path.empty())
            return;
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
        Vec2 pos  = m_Window->GetWindowPosition();
        Vec2 size = m_Window->GetWindowSize();
        if(FILE* f = fopen(path.c_str(), "w"))
        {
            fprintf(f, "%d %d %d %d %d %d\n", (int)pos.x, (int)pos.y, (int)size.x, (int)size.y,
                    OS::Get().IsWindowMaximised() ? 1 : 0, OS::Get().IsWindowFullscreen() ? 1 : 0);
            fclose(f);
        }
#endif
    }

    void Application::RestoreWindowState()
    {
#if !defined(LUMOS_PLATFORM_IOS) && !defined(LUMOS_PLATFORM_ANDROID)
        if(!m_RememberWindow || !m_Window)
            return;
        std::string path = WindowStatePath(m_ProjectSettings.Title);
        FILE* f          = path.empty() ? nullptr : fopen(path.c_str(), "r");
        if(!f)
            return;
        int x = 0, y = 0, w = 0, h = 0, maximised = 0, fullscreen = 0;
        int n = fscanf(f, "%d %d %d %d %d %d", &x, &y, &w, &h, &maximised, &fullscreen);
        fclose(f);
        if(n < 4 || w < 200 || h < 150 || w > 16384 || h > 16384)
            return;
        if(fullscreen)
        {
            OS::Get().SetWindowFullscreen(true);
            return;
        }
        m_Window->SetWindowSize((u32)w, (u32)h);
        // a display that's gone would leave the window off screen
        if(x > -1000 && y > -1000 && x < 8000 && y < 8000)
            m_Window->SetWindowPosition(x, y);
        if(maximised)
            OS::Get().MaximiseWindow();
#endif
    }

    void Application::OnQuit()
    {
        LUMOS_PROFILE_FUNCTION();
        SaveWindowState();
        Serialise();

        MutexDestroy(m_EventQueueMutex);
        MutexDestroy(m_MainThreadQueueMutex);

        ReleaseUndo();

        if(m_ScreenUI)
        {
            UI_State* main = GetUIState();
            delete m_SplashTexture;
            m_SplashTexture = nullptr;
            UIDestroyContext(m_ScreenUI);
            UISetContext(main);
            m_ScreenUI = nullptr;
            ArenaRelease(m_ScreenUIArena);
            m_ScreenUIArena = nullptr;
        }

        DebugMenu::Get().Shutdown();
        DebugMenu::Release();
        AsyncImageLoader::Shutdown();
        AudioTagReader::Shutdown();
        AsyncFileCopy::Shutdown();
        AsyncFileList::Shutdown();

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

    Vec2 Application::GetGameViewSize() const
    {
        if(m_SceneViewWidth > 0 && m_SceneViewHeight > 0)
            return Vec2((float)m_SceneViewWidth, (float)m_SceneViewHeight);
        return GetWindowSize();
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
            if(m_ImGuiManager)
            {
                ImGuiIO& io  = ImGui::GetIO();
                io.DeltaTime = Maths::Max(0.0001f, (float)(ts.GetUnscaledMillis() * 0.001));
            }

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
        UpdateDynamicResolution((float)ts.GetRawMillis());

        ArenaClear(m_FrameArena);

        // The first scene waits for the window to finish going fullscreen (macOS): see below.
        bool holdFirstScene = false;
#ifdef LUMOS_PLATFORM_MACOS
        const bool wantFullscreen = m_ProjectSettings.Fullscreen || GetAppType() == AppType::Editor;
        // 0 = toggle pending, 1 = confirming it entered, 2 = done.
        static int s_FullscreenStage    = 0;
        static int s_FullscreenFrame    = 0;
        static int s_FullscreenAttempts = 0;
        static TimeStamp s_FullscreenAsked;
        static TimeStamp s_FullscreenBegan;
        if(wantFullscreen && s_FullscreenStage < 2 && m_Window)
        {
            s_FullscreenFrame++;
            const TimeStamp clock = Timer::Now();
            static TimeStamp s_FullscreenWaiting = clock;
            const float waited                   = Timer::Duration(s_FullscreenWaiting, clock, 1000.0f);
            // Not straight away, and not before the app is the active one. Asked in the first moments after the
            // window is shown, the move starts and is turned back within a frame or two (measured: asked at once
            // it failed every time, 0.4 s in it took first time) - and then only a retry gets there.
            if(s_FullscreenStage == 0 && s_FullscreenFrame >= 2 && waited >= 400.0f && (OS::Get().IsAppActive() || waited > 1500.0f))
            {
                OS::Get().SetWindowFullscreen(true);
                s_FullscreenStage    = 1;
                s_FullscreenAttempts = 1;
                s_FullscreenAsked    = clock;
                s_FullscreenBegan    = clock;
            }
            else if(s_FullscreenStage == 1)
            {
                if(OS::Get().DidEnterFullscreen())
                    s_FullscreenStage = 2;
                // A move that is under way has the fullscreen style on the window. Without it this long after asking,
                // the request was dropped or turned back: ask again now rather than wait out a long timeout.
                else if(!OS::Get().IsWindowFullscreen() && Timer::Duration(s_FullscreenAsked, clock, 1000.0f) > 400.0f)
                {
                    if(s_FullscreenAttempts < 12)
                    {
                        OS::Get().SetWindowFullscreen(true);
                        s_FullscreenAttempts++;
                        s_FullscreenAsked = clock;
                    }
                    else
                        s_FullscreenStage = 2; // give up rather than loop forever
                }
                // started but never reported done
                else if(Timer::Duration(s_FullscreenBegan, clock, 1000.0f) > 6000.0f)
                    s_FullscreenStage = 2;
            }

            // Loading the first scene now would stall the slide into fullscreen for seconds and then build every
            // render target twice (windowed size, then fullscreen). Light frames until the window is where it is going.
            holdFirstScene = s_FullscreenStage < 2 && m_SceneManager->GetSwitchingScene() && m_SceneManager->GetCurrentScene() == nullptr;
        }
#endif

        static bool s_HoldingFirstScene = false;
        if(holdFirstScene)
        {
            if(!s_HoldingFirstScene)
            {
                s_HoldingFirstScene = true;
                Busy::Begin("Starting");
            }
            PumpBusyFrame(false);
            return m_CurrentState != AppState::Closing;
        }
        else if(s_HoldingFirstScene)
        {
            s_HoldingFirstScene = false;
            Busy::End();
        }

        if(m_SceneManager->GetSwitchingScene())
        {
            LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
            // Something on screen before the load takes the main thread; loaders
            // can keep it moving with Busy::Pump().
            const bool showLoading = m_SceneManager->GetCurrentScene() != nullptr && m_CurrentState == AppState::Running;
            if(showLoading)
            {
                Busy::Begin("Loading scene");
                PumpBusyFrame(true);
            }
            Graphics::Renderer::GetGraphicsContext()->WaitIdle();
            m_SceneManager->ApplySceneSwitch();
            if(showLoading)
                Busy::End();
            // a scene is up: the splash has done its job
            if(m_SplashTexture)
            {
                delete m_SplashTexture;
                m_SplashTexture = nullptr;
            }
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

        // From here to the end of the function a frame is being built and drawn.
        struct FrameScope
        {
            bool& InFrame;
            FrameScope(bool& flag)
                : InFrame(flag)
            {
                InFrame = true;
            }
            ~FrameScope() { InFrame = false; }
        } frameScope(m_InFrame);
        m_SceneDrawnThisFrame = false;

        {
            LUMOS_PROFILE_SCOPE("ImGui::NewFrame");
            if(m_ImGuiManager)
                ImGui::NewFrame();
        }

        // The UI runs on unscaled time so the overlay still animates when the
        // game is paused or slowed down.
        const float uiDelta = Maths::Max(0.0001f, (float)(ts.GetUnscaledMillis() * 0.001));

        Vec2 frameSize = { (float)m_SceneViewWidth, (float)m_SceneViewHeight };
        UIBeginFrame(frameSize, uiDelta, m_SceneViewPosition);
        UIBeginBuild();

        // In the editor the overlay belongs to the game being played, not to editing.
        if(GetAppType() == AppType::Editor)
            DebugMenu::Get().SetEnabled(m_EditorState != EditorState::Preview);

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
                if(m_ImGuiManager)
                    ImGui::Render();
                return m_CurrentState != AppState::Closing;
            }
        }
        if(m_ImGuiManager)
            m_ImGuiManager->OnNewFrame();

#ifdef LUMOS_PLATFORM_IOS
        if(m_ImGuiManager)
        {
            const bool isEditor = GetAppType() == AppType::Editor;
            SafeAreaInsets sa = UIResolveSafeAreaInsets(OS::Get().GetSafeAreaInsets(), isEditor);
            ImGuiViewportP* vp = (ImGuiViewportP*)ImGui::GetMainViewport();

            const ImVec2 vpPos  = vp->Pos;
            const ImVec2 vpSize = vp->Size;

            // (not over a screen UI that draws to the edges itself: ImGui is drawn after it)
            const bool fullBleed = m_ScreenUI && m_ScreenUI->FullBleed;
            if(isEditor && !fullBleed && (sa.left > 0.0f || sa.right > 0.0f || sa.top > 0.0f || sa.bottom > 0.0f))
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

        if(Busy::IsBusy() && !m_ScreenUIEnabled)
            Busy::Draw(false);

        UIEndBuild();
        UILayout();
        UIAnimate();

        UIEndFrame(Graphics::Font::GetDefaultFont());

        BuildScreenUI(uiDelta);

        if(!m_Minimized)
        {
            LUMOS_PROFILE_SCOPE("Application::Render");
            Engine::Get().ResetStats();

            OnRender();
            if(m_ScreenUIEnabled && m_ScreenUI)
                m_SceneRenderer->ScreenUIPass(m_ScreenUI, !m_SceneDrawnThisFrame);
            if(m_ImGuiManager)
                m_ImGuiManager->OnRender(m_SceneManager->GetCurrentScene());

            // Clears debug line and point lists
            DebugRenderer::Reset((float)ts.GetSeconds());

            Graphics::Pipeline::DeleteUnusedCache();
            Graphics::Framebuffer::DeleteUnusedCache();
            Graphics::RenderPass::DeleteUnusedCache();

            m_AssetManager->Update((float)ts.GetElapsedSeconds());
            m_Frames++;
        }
        else if(m_ImGuiManager)
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

        // Submit + present before waiting on physics: the frame was recorded from last frame's
        // synced transforms, so the GPU work does not depend on this update. The submit (often
        // blocked on the swapchain) and the physics job now run at the same time.
        if(!m_Minimized)
            Graphics::Renderer::GetRenderer()->Present();

        // Sync transforms from physics for the next frame
        {
            System::JobSystem::Wait(context);

            m_SystemManager->GetSystem<LumosPhysicsEngine>()->SyncTransforms(m_SceneManager->GetCurrentScene());
            m_SystemManager->GetSystem<B2PhysicsEngine>()->SyncTransforms(m_SceneManager->GetCurrentScene());
        }

        if(!m_Minimized)
            OnDebugDraw(); // after the physics sync: drawn with next frame

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
            m_SceneDrawnThisFrame = true;
        }
    }

    static void ScaleUIStyleDefaults(float dpiScale)
    {
        if(dpiScale <= 1.0f)
            return;
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

    void Application::EnableScreenUI(bool enable)
    {
        m_ScreenUIEnabled = enable;
        if(enable)
            EnsureScreenUI();
    }

    void Application::EnsureScreenUI()
    {
        if(m_ScreenUI || !GetUIState())
            return;

        UI_State* main  = GetUIState();
        m_ScreenUIArena = ArenaAlloc(Megabytes(8));
        m_ScreenUI      = UICreateContext(m_ScreenUIArena);

        UISetContext(m_ScreenUI);
        UIApplyDarkTheme();
        m_ScreenUI->DPIScale   = main->DPIScale;
        m_ScreenUI->InputScale = main->InputScale;
        ScaleUIStyleDefaults(main->DPIScale);
        UISetContext(main);
    }

    void Application::BuildScreenUI(float dt)
    {
        if(!m_ScreenUIEnabled || !m_Window)
            return;
        EnsureScreenUI();
        if(!m_ScreenUI)
            return;

        UI_State* main         = GetUIState();
        m_ScreenUI->DPIScale   = main->DPIScale;
        m_ScreenUI->InputScale = main->InputScale;

        UISetContext(m_ScreenUI);
        UIBeginFrame(Vec2((float)m_Window->GetWidth(), (float)m_Window->GetHeight()), dt, Vec2(0.0f, 0.0f));
        UIBeginBuild();

        OnScreenUI();
        if(Busy::IsBusy())
            Busy::Draw(false);

        UIEndBuild();
        UILayout();
        UIAnimate();
        UIEndFrame(Graphics::Font::GetDefaultFont());
        UISetContext(main);
    }

    void Application::PumpBusyFrame(bool force)
    {
#if defined(LUMOS_PLATFORM_IOS) || defined(LUMOS_PLATFORM_ANDROID)
        // the OS drives the frame there: an extra present from inside one is not ours to make
        (void)force;
        return;
#else
        // Only between frames: a frame that is already being recorded cannot be presented from here.
        if(m_InFrame || !m_Window || !m_SceneRenderer || m_Minimized || m_HeadlessMode || !GetUIState())
            return;
        if(std::this_thread::get_id() != s_MainThreadId)
            return;
        if(m_CurrentState == AppState::Closing || !Graphics::Font::GetDefaultFont())
            return;

        static TimeStamp s_LastPump;
        static bool s_HasPumped = false;
        const TimeStamp now     = Timer::Now();
        if(!force && s_HasPumped && Timer::Duration(s_LastPump, now, 1000.0f) < 33.0f)
            return;
        s_LastPump  = now;
        s_HasPumped = true;

        m_InFrame = true; // also stops this re-entering through an event handler

        // keep the window answering the OS while the main loop is held up
        Input::Get().ResetPressed();
        Input::Get().ResetGestures();
        m_Window->ProcessInput();
        ApplyPendingWindowResize();

        EnsureScreenUI();
        if(m_ScreenUI && Graphics::Renderer::GetRenderer()->Begin())
        {
            UI_State* main         = GetUIState();
            m_ScreenUI->DPIScale   = main->DPIScale;
            m_ScreenUI->InputScale = main->InputScale;

            UISetContext(m_ScreenUI);
            UIBeginFrame(Vec2((float)m_Window->GetWidth(), (float)m_Window->GetHeight()), 0.033f, Vec2(0.0f, 0.0f));
            UIBeginBuild();
            Busy::Draw(true);
            UIEndBuild();
            UILayout();
            UIAnimate();
            UIEndFrame(Graphics::Font::GetDefaultFont());
            UISetContext(main);

            m_SceneRenderer->ScreenUIPass(m_ScreenUI, true);
            Graphics::Renderer::GetRenderer()->Present();
        }

        m_InFrame = false;
#endif
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

    void Application::OnBackgroundUpdate(float dt)
    {
        if(m_CurrentState != AppState::Running || !m_SceneManager)
            return;
        LuaManager::Get().OnBackgroundUpdate(m_SceneManager->GetCurrentScene(), dt);
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
        if(m_ImGuiManager)
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
        // both UIs get the keys; only the one with a focused field does anything with them
        UI_State* screenUI = m_ScreenUIEnabled ? m_ScreenUI : nullptr;
        dispatcher.Dispatch<KeyTypedEvent>([screenUI](KeyTypedEvent& event) -> bool
        {
            UIProcessKeyTyped(event.Character);
            if(screenUI)
            {
                UI_State* main = GetUIState();
                UISetContext(screenUI);
                UIProcessKeyTyped(event.Character);
                UISetContext(main);
            }
            return false;
        });
        dispatcher.Dispatch<KeyPressedEvent>([screenUI](KeyPressedEvent& event) -> bool
        {
            UIProcessKeyPressed(event.GetKeyCode());
            if(screenUI)
            {
                UI_State* main = GetUIState();
                UISetContext(screenUI);
                UIProcessKeyPressed(event.GetKeyCode());
                UISetContext(main);
            }
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
        // Run them outside the lock: a job can take a while (and pump frames),
        // and anything it submits must not wait on its own caller.
        TDArray<Function<void()>> pending;
        {
            ScopedMutex lock(m_MainThreadQueueMutex);
            if(m_MainThreadQueue.Empty())
                return;
            pending = std::move(m_MainThreadQueue);
            m_MainThreadQueue.Clear();
        }

        for(const auto& func : pending)
            func();
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

    void Application::SetWindowTransparent(bool transparent)
    {
        if(!m_Window || transparent == m_WindowTransparent)
            return;
        m_WindowTransparent = transparent;
        m_Window->SetTransparent(transparent);
        if(m_Window->GetSwapChain())
            m_Window->GetSwapChain()->SetTransparent(transparent);
        if(m_SceneRenderer)
            m_SceneRenderer->SetTransparentBackground(transparent);

        // composite alpha is fixed at swapchain creation, so force a same-size rebuild
        if(!m_HasPendingWindowResize)
        {
            m_PendingWindowWidth    = m_Window->GetWidth();
            m_PendingWindowHeight   = m_Window->GetHeight();
            m_PendingWindowDPIScale = m_Window->GetDPIScale();
        }
        m_HasPendingWindowResize = true;
        m_ForceWindowResize      = true;
    }

    void Application::SetQualityLevel(int level)
    {
        const auto& caps = Graphics::Renderer::GetCapabilities();
        uint8_t use      = level >= 0 ? (uint8_t)Maths::Min(level, 2) : QualitySettings::DetectLevel(caps.GPUType, caps.VideoMemoryMB, caps.Renderer.c_str());
        m_QualitySettings.SetGeneralLevel(use);
        // a weak GPU at medium still gets the cheap forward shader
        if(level < 0 && use == 1 && caps.GPUType == 1)
            m_QualitySettings.LowForwardShader = true;

        const uint32_t width  = m_Window ? m_Window->GetWidth() : 0;
        const uint32_t height = m_Window ? m_Window->GetHeight() : 0;
        m_QualitySettings.MaxScale     = 1.0f;
        const float scale              = QualitySettings::ScaleForBudget(use, width, height, m_QualitySettings.MinScale);
        const bool changed             = Maths::Abs(scale - m_QualitySettings.RendererScale) > 0.001f;
        m_QualitySettings.RendererScale = scale;
        LINFO("Quality level %d (%s, %s) render scale %.2f at %ux%u", (int)use, level >= 0 ? "forced" : "detected", caps.Renderer.c_str(), scale, width, height);
        if(changed && m_SceneRenderer && width > 0 && height > 0)
            OnSceneViewSizeUpdated(width, height);
    }

    // Steps the render scale down when frames run long and probes back up once they are comfortable
    void Application::UpdateDynamicResolution(float frameMs)
    {
        if(!m_QualitySettings.DynamicResolution || m_Minimized || !m_Window || !m_SceneRenderer)
            return;

        static float s_Ema         = 16.7f;
        static float s_SinceChange = 0.0f;
        static float s_GoodTime    = 0.0f;
        static float s_Ceiling     = 1.0f; // a scale that was too slow caps the probe back up

        // a loading stall or resize hitch is not steady load: forget it and wait before judging again
        if(frameMs > 250.0f)
        {
            s_Ema         = m_QualitySettings.TargetFrameMs;
            s_SinceChange = 0.0f;
            s_GoodTime    = 0.0f;
            return;
        }
        const float dt = frameMs;
        s_Ema += (dt - s_Ema) * 0.05f;
        s_SinceChange += dt * 0.001f;
        const float target = m_QualitySettings.TargetFrameMs;
        s_GoodTime         = s_Ema < target * 1.05f ? s_GoodTime + dt * 0.001f : 0.0f;
        if(s_GoodTime > 60.0f)
        {
            s_Ceiling = Maths::Min(1.0f, s_Ceiling + 0.05f);
            s_GoodTime = 30.0f;
        }
        if(s_SinceChange < 2.5f)
            return;

        const float current = m_QualitySettings.RendererScale;
        float next          = current;
        if(s_Ema > target * 1.2f && current > m_QualitySettings.MinScale + 0.001f)
        {
            next      = Maths::Max(m_QualitySettings.MinScale, current - 0.1f);
            s_Ceiling = current - 0.01f;
        }
        else if(s_GoodTime > 10.0f && current + 0.05f <= Maths::Min(m_QualitySettings.MaxScale, s_Ceiling) + 0.001f)
            next = current + 0.05f;

        if(Maths::Abs(next - current) < 0.001f)
            return;

        m_QualitySettings.RendererScale = next;
        s_SinceChange                   = 0.0f;
        s_GoodTime                      = 0.0f;
        s_Ema                           = target;
        LINFO("Dynamic resolution: %.2f -> %.2f", current, next);
        OnSceneViewSizeUpdated(m_Window->GetWidth(), m_Window->GetHeight());
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
        if(width != 0 && height != 0 && width == m_AppliedWindowWidth && height == m_AppliedWindowHeight && !m_ForceWindowResize)
            return;
        m_ForceWindowResize = false;

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
        // quit before start-up finished (iOS can terminate an app that is still launching)
        if(!m_SceneManager || !m_ProjectLoaded)
            return;

        // A test run must leave the project exactly as it found it - otherwise
        // its --width/--height end up committed to the .lmproj.
        // a shipped app's project lives in its read-only bundle
        const std::string& root = m_ProjectSettings.m_ProjectRoot;
        if(root.find(".app/Contents/") != std::string::npos || root.find(".app/") != std::string::npos)
        {
            LINFO("Bundled project - skipping project serialisation");
            return;
        }
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

            // An exported app has only the pack (in a read-only bundle on iOS, a signed one on macOS):
            // //Assets/ writes (saves, settings) go to a per-user folder that reads check before the pack
            if(m_ProjectSettings.Packaged)
            {
                std::string overlay = WritableRoot() + "Assets";
                FileSystem::CreateFolderIfDoesntExist(Str8StdS(overlay));
                m_AssetPath = Str8F(m_AssetPath, "%s", overlay.c_str());
                FileSystem::Get().SetAssetPath(m_AssetPath);
                LINFO("Writable assets: %s", overlay.c_str());
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

    std::string Application::WritableRoot() const
    {
        if(m_ProjectSettings.Packaged)
            return FileSystem::UserDataPath(m_ProjectSettings.m_ProjectName) + "/";
        return m_ProjectSettings.m_ProjectRoot;
    }

    void Application::CreateAssetFolders()
    {
        // an exported app's Assets live in the pack; writes go to WritableRoot()
        if(m_ProjectSettings.Packaged)
            return;
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
