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

// Needed for iOS build
#if defined(LUMOS_PLATFORM_IOS)
#include <string>
#endif

namespace Lumos
{
    class Timer;
    class Window;
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
        void OpenProject(const std::string& filePath);
        void OpenNewProject(const std::string& path, const std::string& name = "New Project");

        virtual void OnEvent(Event& e);
        virtual void OnNewScene(Scene* scene);
        virtual void OnRender();
        virtual void OnUpdate(const TimeStep& dt);
        virtual void OnImGui();
        virtual void OnDebugDraw();
        
        virtual void ExitApp();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
        Graphics::SceneRenderer* GetSceneRenderer() const { return m_SceneRenderer.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }
        EditorState GetEditorState() const { return m_EditorState; }
        SystemManager* GetSystemManager() const { return m_SystemManager.get(); }
        Scene* GetCurrentScene() const;
        ImGuiManager* GetImGuiManager() const { return m_ImGuiManager.get(); }

        void SetAppState(AppState state) { m_CurrentState = state; }
        void SetEditorState(EditorState state) { m_EditorState = state; }
        void SetSceneActive(bool active) { m_SceneActive = active; }
        void SetDisableMainSceneRenderer(bool disable) { m_DisableMainSceneRenderer = disable; }
        bool GetSceneActive() const { return m_SceneActive; }

        Vec2 GetWindowSize() const;
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
        virtual void Deserialise();

        void MountFileSystemPaths();
        void CreateAssetFolders();
        void SetEngineAssetPath();

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
            bool DefaultIcon  = true;
            bool HideTitleBar = false;
        };

        ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }

        Arena* GetFrameArena() const { return m_FrameArena; }
        static void UpdateSystems();

        Vec2 m_SceneViewPosition; // For Editor
        const String8& GetAssetPath() const { return m_AssetPath; };

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
        AppType m_AppType         = AppType::Editor;

        TDArray<Function<void()>> m_MainThreadQueue;
        Mutex* m_MainThreadQueueMutex;

        Arena* m_FrameArena;
        Arena* m_Arena;
        Arena* m_UIArena;

        QualitySettings m_QualitySettings;
        StringPool* m_StringPool;

        String8 m_AssetPath;

        NONCOPYABLE(Application)
    };

    // Defined by client
    Application* CreateApplication();
}
