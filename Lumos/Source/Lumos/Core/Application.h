#pragma once
#include "Core/Reference.h"
#include "Scene/SceneManager.h"
#include "Scene/SystemManager.h"
#include "Core/OS/FileSystem.h"
#include "Core/QualitySettings.h"
#include "Maths/MathsFwd.h"
#include "Maths/Vector2.h"
#include "Core/Function.h"
#include "Core/Mutex.h"
#include <string>

namespace Lumos
{
    class Timer;
    class Window;
    struct UI_State;
    struct WindowDesc;
    class AudioManager;
    class SystemManager;
    class Editor;
    class Scene;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class ImGuiManager;
    class AssetManager;
    class StringPool;

    namespace Graphics
    {
        class SceneRenderer;
        class Texture2D;
        enum class RenderAPI : uint32_t;
    }

    enum class AppState
    {
        Running,
        Loading,
        Closing
    };

    enum class EditorState
    {
        Paused,
        Play,
        Next,
        Preview
    };

    enum class AppType
    {
        Game,
        Editor
    };

    class LUMOS_EXPORT Application : public TSingleton<Application>
    {
        friend class TSingleton<Application>;
        friend class Editor;
        friend class Runtime;
        template <typename Archive>
        friend void save(Archive& archive, const Application& application);

        template <typename Archive>
        friend void load(Archive& archive, Application& application);

    public:
        Application();
        virtual ~Application();

        virtual void Init();
        virtual void OnQuit();

        void Run();
        bool OnFrame();

        void OnExitScene();
        void OnSceneViewSizeUpdated(uint32_t width, uint32_t height);

        // level 0..2 forces it, -1 picks from the GPU. Scale follows the level's pixel budget.
        void SetQualityLevel(int level);
        void UpdateDynamicResolution(float frameMs);
        void OpenProject(const std::string& filePath);
        void OpenNewProject(const std::string& path, const std::string& name = "New Project");

        virtual void OnEvent(Event& e);
        virtual void OnNewScene(Scene* scene);
        virtual void OnRender();
        virtual void OnUpdate(const TimeStep& dt);
        // no frames while backgrounded (iOS): scripts still get OnBackgroundUpdate from a timer
        void OnBackgroundUpdate(float dt);
        virtual void OnImGui();
        virtual void OnDebugDraw();

        // A second UI the size of the whole window, drawn over the scene (tools,
        // editor panels). Built in OnScreenUI with that UI as the current context.
        void EnableScreenUI(bool enable);
        bool IsScreenUIEnabled() const { return m_ScreenUIEnabled; }
        UI_State* GetScreenUI() const { return m_ScreenUI; }
        // The picture shown at startup, kept until the first scene is in so loading frames can carry on from it. Null after.
        Graphics::Texture2D* GetSplashTexture() const { return m_SplashTexture; }
        virtual void OnScreenUI() { }

        // Draws one frame holding only the busy overlay (Core/BusyOverlay.h). No-op
        // inside a normal frame or off the main thread; force skips the rate limit.
        void PumpBusyFrame(bool force = false);
        bool IsInFrame() const { return m_InFrame; }

        virtual void ExitApp();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
        Graphics::SceneRenderer* GetSceneRenderer() const { return m_SceneRenderer.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }
        EditorState GetEditorState() const { return m_EditorState; }
        AppType GetAppType() const { return m_AppType; }
        SystemManager* GetSystemManager() const { return m_SystemManager.get(); }
        Scene* GetCurrentScene() const;
        ImGuiManager* GetImGuiManager() const { return m_ImGuiManager.get(); }
        // Before Init: false = no ImGui at all (no context, nothing drawn). The editor runs without it.
        static void SetImGuiEnabled(bool enabled);
        static bool IsImGuiEnabled();

        void SetAppState(AppState state) { m_CurrentState = state; }
        void SetEditorState(EditorState state) { m_EditorState = state; }
        void SetSceneActive(bool active) { m_SceneActive = active; }
        void SetDisableMainSceneRenderer(bool disable) { m_DisableMainSceneRenderer = disable; }
        bool GetDisableMainSceneRenderer() const { return m_DisableMainSceneRenderer; }
        bool GetSceneActive() const { return m_SceneActive; }

        Vec2 GetWindowSize() const;
        // the area the game draws into: the window, or the editor's Game tab (same units as the window)
        Vec2 GetGameViewSize() const;
        Vec2 GetGameViewPosition() const { return m_SceneViewPosition; }
        float GetWindowDPI() const;

        SharedPtr<AssetManager>& GetAssetManager();

        const QualitySettings& GetQualitySettings() const { return m_QualitySettings; }
        QualitySettings& GetQualitySettings() { return m_QualitySettings; }

        void SubmitToMainThread(const Function<void()>& function);
        void ExecuteMainThreadQueue();

        template <typename T>
        T* GetSystem()
        {
            return m_SystemManager->GetSystem<T>();
        }

        template <typename Func>
        void QueueEvent(Func&& func)
        {
            m_EventQueue.PushBack(func);
        }

        template <typename TEvent, bool Immediate = false, typename... TEventArgs>
        void DispatchEvent(TEventArgs&&... args)
        {
            SharedPtr<TEvent> event = CreateSharedPtr<TEvent>(Forward<TEventArgs>(args)...);
            if(Immediate)
            {
                OnEvent(*event);
            }
            else
            {
                ScopedMutex lock(m_EventQueueMutex);
                m_EventQueue.PushBack([event]()
                                      { Application::Get().OnEvent(*event); });
            }
        }

        bool OnWindowResize(WindowResizeEvent& e);

        // Applies the latest pending window size, if any. Called once per frame,
        // before anything renders.
        void ApplyPendingWindowResize();

        // See-through window: desktop shows wherever nothing was drawn (premultiplied alpha).
        // The swapchain is rebuilt at the top of the next frame.
        void SetWindowTransparent(bool transparent);
        bool IsWindowTransparent() const { return m_WindowTransparent; }

        void SetSceneViewDimensions(uint32_t width, uint32_t height)
        {
            if(width != m_SceneViewWidth)
            {
                m_SceneViewWidth       = width;
                m_SceneViewSizeUpdated = true;
            }

            if(height != m_SceneViewHeight)
            {
                m_SceneViewHeight      = height;
                m_SceneViewSizeUpdated = true;
            }
        }

        void TestUI();

        virtual void Serialise();
        void SaveWindowState();
        void RestoreWindowState();
        virtual void Deserialise();

        void MountFileSystemPaths();
        void CreateAssetFolders();
        void SetEngineAssetPath();

        void SetScreenshotPath(const std::string& path) { m_ScreenshotPath = path; m_TakeScreenshotOnInit = true; }

        // Capture the next presented frame to `path`, without exiting after.
        void RequestScreenshot(const std::string& path)
        {
            m_ScreenshotPath        = path;
            m_TakeScreenshotOnInit  = true;
            m_ScreenshotTaken       = false;
            m_ScreenshotFrameDelay  = 0;
            m_ScreenshotFrameTarget = 1;
            m_CloseAfterScreenshot  = false;
        }

        // Static so it survives the Application being deleted before main() returns.
        static void SetExitCode(int code) { s_ExitCode = code; }
        static int GetExitCode() { return s_ExitCode; }

        enum class SafeAreaLayout : int
        {
            Fullscreen       = 0, // scene + UI edge-to-edge
            SafeArea         = 1, // scene render + UI inset (letterboxed)
            FullscreenSafeUI = 2, // scene edge-to-edge, UI inset only
        };

        struct ProjectSettings
        {
            std::string m_ProjectRoot;
            std::string m_ProjectName;
            std::string m_EngineAssetPath; // TODO: move
            uint32_t Width = 1200, Height = 800;
            bool Fullscreen        = true;
            bool VSync             = true;
            bool Borderless        = false;
            bool ShowConsole       = true;
            std::string Title      = "App";
            int RenderAPI          = 1;
            int ProjectVersion     = 1;
            int8_t DesiredGPUIndex = -1;
            std::string IconPath;
            bool DefaultIcon       = true;
            bool HideTitleBar      = false;
            std::string AppScript; // VFS path to auto-load Lua script (tools only, ignored for games)
            std::string BundleIdentifier; // "com.company.game"
            std::string Version     = "1.0.0";
            std::string BuildNumber = "1";
            std::string SplashPath; // VFS path to custom splash image
            float SplashBGColour[4] = { 40.0f / 256.0f, 42.0f / 256.0f, 54.0f / 256.0f, 1.0f };
            std::string StartScene; // Scene name to load on startup (v11+, replaces SceneIndex)
            bool AutoImportMeshes  = true; // Auto-convert source models to .lmesh on load

            int Orientation        = 0;
            // TARGETED_DEVICE_FAMILY: 1 = iPhone, 2 = iPad, 3 = both
            int DeviceFamily       = 3;
            std::string MinIOSVersion = "16.0";
            bool StatusBarHidden   = true;
            bool UsesNonExemptEncryption = false; // ITSAppUsesNonExemptEncryption
            // Privacy usage descriptions — empty string omits the key
            std::string CameraUsage;
            std::string MicrophoneUsage;
            std::string PhotoLibraryUsage;
            std::string LocationUsage;

            int SafeAreaMode = (int)SafeAreaLayout::FullscreenSafeUI;

            // an exported app (set by the Runtime, not saved): the project sits in a read-only / signed
            // bundle, so everything the engine writes goes under WritableRoot() instead
            bool Packaged = false;
        };

        // where the engine may write project-side files (pipeline cache, imgui.ini, //Assets/ saves):
        // the project root, or a per-user folder for an exported app. Ends in a slash.
        std::string WritableRoot() const;

        ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }

        Arena* GetFrameArena() const { return m_FrameArena; }
        static void UpdateSystems();

        Vec2 m_SceneViewPosition; // For Editor
        const String8& GetAssetPath() const { return m_AssetPath; };

        bool EmbedEngineShaders();

    protected:
        ProjectSettings m_ProjectSettings;
        bool m_ProjectLoaded = false;

    private:
        void AddDefaultScene();

        bool OnWindowClose(WindowCloseEvent& e);
        bool ShouldUpdateSystems = false;

        uint32_t m_Frames               = 0;
        uint32_t m_Updates              = 0;
        float m_SecondTimer             = 0.0f;
        bool m_Minimized                = false;
        bool m_SceneActive              = true;
        bool m_DisableMainSceneRenderer = false;

        uint32_t m_SceneViewWidth  = 0;
        uint32_t m_SceneViewHeight = 0;

        // Latest size reported by the window while a resize is in flight.
        bool m_HasPendingWindowResize    = false;
        uint32_t m_PendingWindowWidth    = 0;
        uint32_t m_PendingWindowHeight   = 0;
        float m_PendingWindowDPIScale    = 1.0f;
        uint32_t m_AppliedWindowWidth    = 0;
        uint32_t m_AppliedWindowHeight   = 0;
        bool m_ForceWindowResize         = false;
        bool m_WindowTransparent         = false;

        bool m_SceneViewSizeUpdated = false;
        bool m_RenderDocEnabled     = false;
        bool m_ImGuiClearScreen     = false;

        Mutex* m_EventQueueMutex;
        TDArray<Function<void()>> m_EventQueue;

        UniquePtr<Window> m_Window;
        UniquePtr<SceneManager> m_SceneManager;
        UniquePtr<SystemManager> m_SystemManager;
        UniquePtr<Graphics::SceneRenderer> m_SceneRenderer;
        UniquePtr<ImGuiManager> m_ImGuiManager;
        UniquePtr<Timer> m_Timer;
        SharedPtr<AssetManager> m_AssetManager;

        AppState m_CurrentState   = AppState::Loading;
        EditorState m_EditorState = EditorState::Preview;
        AppType m_AppType         = AppType::Game;

        TDArray<Function<void()>> m_MainThreadQueue;
        Mutex* m_MainThreadQueueMutex;

        Arena* m_FrameArena;
        Arena* m_Arena;
        Arena* m_UIArena;

        void EnsureScreenUI();
        void BuildScreenUI(float dt);
        UI_State* m_ScreenUI     = nullptr;
        Graphics::Texture2D* m_SplashTexture = nullptr;
        Arena* m_ScreenUIArena   = nullptr;
        bool m_ScreenUIEnabled   = false;
        bool m_InFrame           = false; // between the start of a frame's update and its present
        bool m_SceneDrawnThisFrame = false;

        QualitySettings m_QualitySettings;
        StringPool* m_StringPool;

        String8 m_AssetPath;

        // Screenshot on init feature
        std::string m_ScreenshotPath;
        bool m_TakeScreenshotOnInit = false;
        bool m_ScreenshotTaken      = false;
        int m_ScreenshotFrameDelay  = 0;
        int m_ScreenshotFrameTarget = 5;

        // Command line option flags
        bool m_CloseAfterScreenshot   = true;  // Default: close after screenshot
        bool m_ForceEmbeddedShaders   = false; // Default: auto-detect
        bool m_ShowTestUI             = false; // Default: hidden
        bool m_HeadlessMode           = false; // Default: normal rendering
        bool m_RememberWindow         = true;  // restore size / position / maximised / fullscreen (off for tests, benchmarks)
        bool m_BenchmarkMode          = false; // Default: no benchmark
        int m_BenchmarkFrameCount     = 0;
        int m_BenchmarkCurrentFrame   = 0;
        bool m_ImportAssetsMode       = false; // --import-assets: bake .lmesh caches then quit
        std::string m_InitialSceneName;        // Scene to load on startup

        // Statics only - Runtime/Editor subclass this and the generated Xcode
        // projects don't track header deps, so growing the layout silently
        // breaks them until those TUs are rebuilt.
        static int s_ExitCode;

        void ImportAllProjectAssets();

        NONCOPYABLE(Application)
    };

    // Defined by client
    Application* CreateApplication();
}
