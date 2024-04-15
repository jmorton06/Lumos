#pragma once
#include "Core/Reference.h"
#include "Scene/SceneManager.h"
#include "Scene/SystemManager.h"
#include "Core/OS/FileSystem.h"
#include "Core/QualitySettings.h"

#include <thread>
#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp>
#include <queue>
#include <glm/vec2.hpp>

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

    namespace Graphics
    {
        class RenderPasses;
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

    class LUMOS_EXPORT Application
    {
        friend class Editor;
        friend class Runtime;

    public:
        Application();
        virtual ~Application();

        void Run();
        bool OnFrame();

        void OnExitScene();
        void OnSceneViewSizeUpdated(uint32_t width, uint32_t height);
        void OpenProject(const std::string& filePath);
        void OpenNewProject(const std::string& path, const std::string& name = "New Project");

        virtual void OnQuit();
        virtual void Init();
        virtual void OnEvent(Event& e);
        virtual void OnNewScene(Scene* scene);
        virtual void OnRender();
        virtual void OnUpdate(const TimeStep& dt);
        virtual void OnImGui();
        virtual void OnDebugDraw();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
        Graphics::RenderPasses* GetRenderPasses() const { return m_RenderPasses.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }
        EditorState GetEditorState() const { return m_EditorState; }
        SystemManager* GetSystemManager() const { return m_SystemManager.get(); }
        Scene* GetCurrentScene() const;
        ImGuiManager* GetImGuiManager() const { return m_ImGuiManager.get(); }

        void SetAppState(AppState state) { m_CurrentState = state; }
        void SetEditorState(EditorState state) { m_EditorState = state; }
        void SetSceneActive(bool active) { m_SceneActive = active; }
        void SetDisableMainRenderPasses(bool disable) { m_DisableMainRenderPasses = disable; }
        bool GetSceneActive() const { return m_SceneActive; }

        glm::vec2 GetWindowSize() const;
        float GetWindowDPI() const;

        SharedPtr<AssetManager>& GetAssetManager();

        const QualitySettings& GetQualitySettings() const { return m_QualitySettings; }
        QualitySettings& GetQualitySettings() { return m_QualitySettings; }

        void SubmitToMainThread(const std::function<void()>& function);
        void ExecuteMainThreadQueue();

        static Application& Get() { return *s_Instance; }

        static void Release()
        {
            if(s_Instance)
                delete s_Instance;
            s_Instance = nullptr;
        }

        template <typename T>
        T* GetSystem()
        {
            return m_SystemManager->GetSystem<T>();
        }

        template <typename Func>
        void QueueEvent(Func&& func)
        {
            m_EventQueue.push(func);
        }

        template <typename TEvent, bool Immediate = false, typename... TEventArgs>
        void DispatchEvent(TEventArgs&&... args)
        {
            SharedPtr<TEvent> event = CreateSharedPtr<TEvent>(std::forward<TEventArgs>(args)...);
            if(Immediate)
            {
                OnEvent(*event);
            }
            else
            {
                std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
                m_EventQueue.push([event]()
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

        virtual void Serialise();
        virtual void Deserialise();

        template <typename Archive>
        void save(Archive& archive) const

        {
            int projectVersion = 8;

            archive(cereal::make_nvp("Project Version", projectVersion));

            // Version 1

            std::string path;

            // Window size and full screen shouldnt be in project

            // Version 8 removed width and height
            archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                    cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                    cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                    cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                    cereal::make_nvp("Title", m_ProjectSettings.Title));
            // Version 2

            auto paths = m_SceneManager->GetSceneFilePaths();
            std::vector<std::string> newPaths;
            for(auto& path : paths)
            {
                std::string newPath;
                FileSystem::Get().AbsolutePathToFileSystem(path, newPath);
                newPaths.push_back(path);
            }
            archive(cereal::make_nvp("Scenes", newPaths));
            // Version 3
            archive(cereal::make_nvp("SceneIndex", m_SceneManager->GetCurrentSceneIndex()));
            // Version 4
            archive(cereal::make_nvp("Borderless", m_ProjectSettings.Borderless));
            // Version 5
            archive(cereal::make_nvp("EngineAssetPath", m_ProjectSettings.m_EngineAssetPath));
            // Version 6
            archive(cereal::make_nvp("GPUIndex", m_ProjectSettings.DesiredGPUIndex));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            int sceneIndex = 0;
            archive(cereal::make_nvp("Project Version", m_ProjectSettings.ProjectVersion));

            std::string test;
            if(m_ProjectSettings.ProjectVersion < 8)
            {
                archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                        cereal::make_nvp("Width", m_ProjectSettings.Width),
                        cereal::make_nvp("Height", m_ProjectSettings.Height),
                        cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                        cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                        cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                        cereal::make_nvp("Title", m_ProjectSettings.Title));
            }
            else
            {
                archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                        cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                        cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                        cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                        cereal::make_nvp("Title", m_ProjectSettings.Title));
            }
            if(m_ProjectSettings.ProjectVersion > 2)
            {
                std::vector<std::string> sceneFilePaths;
                archive(cereal::make_nvp("Scenes", sceneFilePaths));

                for(auto& filePath : sceneFilePaths)
                {
                    m_SceneManager->AddFileToLoadList(filePath);
                }

                if(sceneFilePaths.size() == sceneIndex)
                    AddDefaultScene();
            }
            if(m_ProjectSettings.ProjectVersion > 3)
            {
                archive(cereal::make_nvp("SceneIndex", sceneIndex));
                m_SceneManager->SwitchScene(sceneIndex);
            }
            if(m_ProjectSettings.ProjectVersion > 4)
            {
                archive(cereal::make_nvp("Borderless", m_ProjectSettings.Borderless));
            }

            if(m_ProjectSettings.ProjectVersion > 5)
            {
                archive(cereal::make_nvp("EngineAssetPath", m_ProjectSettings.m_EngineAssetPath));
            }
            else
                m_ProjectSettings.m_EngineAssetPath = "/Users/jmorton/dev/Lumos/Lumos/Assets/";

            if(m_ProjectSettings.ProjectVersion > 6)
                archive(cereal::make_nvp("GPUIndex", m_ProjectSettings.DesiredGPUIndex));
        }

        void MountFileSystemPaths();

        struct ProjectSettings
        {
            std::string m_ProjectRoot;
            std::string m_ProjectName;
            std::string m_EngineAssetPath; // TODO: move
            uint32_t Width = 1200, Height = 800;
            bool Fullscreen  = true;
            bool VSync       = true;
            bool Borderless  = false;
            bool ShowConsole = true;
            std::string Title;
            int RenderAPI;
            int ProjectVersion;
            int8_t DesiredGPUIndex = -1;
            std::string IconPath;
            bool DefaultIcon  = true;
            bool HideTitleBar = false;
        };

        struct RenderConfig
        {
            uint32_t IrradianceMapSize  = 64;
            uint32_t EnvironmentMapSize = 1024;
        };

        ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }
        RenderConfig& GetRenderConfigSettings() { return m_RenderConfig; }

        Arena* GetFrameArena() const { return m_FrameArena; }
        static void UpdateSystems();

    protected:
        ProjectSettings m_ProjectSettings;
        RenderConfig m_RenderConfig;
        bool m_ProjectLoaded = false;

    private:
        void AddDefaultScene();

        bool OnWindowClose(WindowCloseEvent& e);
        bool ShouldUpdateSystems = false;

        uint32_t m_Frames              = 0;
        uint32_t m_Updates             = 0;
        float m_SecondTimer            = 0.0f;
        bool m_Minimized               = false;
        bool m_SceneActive             = true;
        bool m_DisableMainRenderPasses = false;

        uint32_t m_SceneViewWidth   = 0;
        uint32_t m_SceneViewHeight  = 0;
        bool m_SceneViewSizeUpdated = false;
        bool m_RenderDocEnabled     = false;

        std::mutex m_EventQueueMutex;
        std::queue<std::function<void()>> m_EventQueue;

        UniquePtr<Window> m_Window;
        UniquePtr<SceneManager> m_SceneManager;
        UniquePtr<SystemManager> m_SystemManager;
        UniquePtr<Graphics::RenderPasses> m_RenderPasses;
        UniquePtr<ImGuiManager> m_ImGuiManager;
        UniquePtr<Timer> m_Timer;
        SharedPtr<AssetManager> m_AssetManager;

        AppState m_CurrentState   = AppState::Loading;
        EditorState m_EditorState = EditorState::Preview;
        AppType m_AppType         = AppType::Editor;

        static Application* s_Instance;

        std::thread m_UpdateThread;

        std::vector<std::function<void()>> m_MainThreadQueue;
        std::mutex m_MainThreadQueueMutex;

        Arena* m_FrameArena;
        Arena* m_Arena;

        QualitySettings m_QualitySettings;

        NONCOPYABLE(Application)
    };

    // Defined by client
    Application* CreateApplication();
}
