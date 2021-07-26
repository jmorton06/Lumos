#pragma once
#include "Core/Reference.h"
#include "Scene/SceneManager.h"
#include "Scene/SystemManager.h"
#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp>

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

    namespace Graphics
    {
        class RenderGraph;
        enum class RenderAPI : uint32_t;
    }

    namespace Maths
    {
        class Vector2;
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
        void OpenNewProject(const std::string& path);

        virtual void OnQuit();
        virtual void Init();
        virtual void OnEvent(Event& e);
        virtual void OnNewScene(Scene* scene);
        virtual void OnRender();
        virtual void OnUpdate(const TimeStep& dt);
        virtual void OnImGui();
        virtual void OnDebugDraw();

        SceneManager* GetSceneManager() const
        {
            return m_SceneManager.get();
        }

        Graphics::RenderGraph* GetRenderGraph() const
        {
            return m_RenderGraph.get();
        }

        Window* GetWindow() const
        {
            return m_Window.get();
        }

        AppState GetState() const
        {
            return m_CurrentState;
        }

        EditorState GetEditorState() const
        {
            return m_EditorState;
        }

        SystemManager* GetSystemManager() const
        {
            return m_SystemManager.get();
        }

        Scene* GetCurrentScene() const;

        void SetAppState(AppState state)
        {
            m_CurrentState = state;
        }

        void SetEditorState(EditorState state)
        {
            m_EditorState = state;
        }

        void SetSceneActive(bool active)
        {
            m_SceneActive = active;
        }

        bool GetSceneActive() const
        {
            return m_SceneActive;
        }

        Maths::Vector2 GetWindowSize() const;
        float GetWindowDPI() const;

        SharedRef<ShaderLibrary>& GetShaderLibrary();

        static Application& Get()
        {
            return *s_Instance;
        }

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

        bool OnWindowResize(WindowResizeEvent& e);

        void SetSceneViewDimensions(uint32_t width, uint32_t height)
        {
            if(width != m_SceneViewWidth)
            {
                m_SceneViewWidth = width;
                m_SceneViewSizeUpdated = true;
            }

            if(height != m_SceneViewHeight)
            {
                m_SceneViewHeight = height;
                m_SceneViewSizeUpdated = true;
            }
        }

        void EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName);

        virtual void Serialise();
        virtual void Deserialise();

        template <typename Archive>
        void save(Archive& archive) const

        {
            int projectVersion = 5;

            archive(cereal::make_nvp("Project Version", projectVersion));
            auto windowSize = GetWindowSize() / GetWindowDPI();

            if(windowSize.x == 0)
                windowSize.x = 800;

            if(windowSize.y == 0)
                windowSize.y = 600;

            //Version 1

            std::string path;

            archive(cereal::make_nvp("RenderAPI", RenderAPI),
                cereal::make_nvp("Width", (int)windowSize.x),
                cereal::make_nvp("Height", (int)windowSize.y),
                cereal::make_nvp("Fullscreen", Fullscreen),
                cereal::make_nvp("VSync", VSync),
                cereal::make_nvp("ShowConsole", ShowConsole),
                cereal::make_nvp("Title", Title));
            //Version 2

            auto paths = m_SceneManager->GetSceneFilePaths();
            std::vector<std::string> newPaths;
            for(auto& path : paths)
            {
                std::string newPath;
                VFS::Get()->AbsoulePathToVFS(path, newPath);
                newPaths.push_back(path);
            }
            archive(cereal::make_nvp("Scenes", newPaths));
            //Version 3
            archive(cereal::make_nvp("SceneIndex", m_SceneManager->GetCurrentSceneIndex()));
            //Version 4
            archive(cereal::make_nvp("Borderless", Borderless));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            int projectVersion = 0;
            int sceneIndex = 0;
            archive(cereal::make_nvp("Project Version", projectVersion));

            std::string test;
            if(projectVersion > 0)
            {
                archive(cereal::make_nvp("RenderAPI", RenderAPI),
                    cereal::make_nvp("Width", Width),
                    cereal::make_nvp("Height", Height),
                    cereal::make_nvp("Fullscreen", Fullscreen),
                    cereal::make_nvp("VSync", VSync),
                    cereal::make_nvp("ShowConsole", ShowConsole),
                    cereal::make_nvp("Title", Title));
            }
            if(projectVersion > 2)
            {
                std::vector<std::string> sceneFilePaths;
                archive(cereal::make_nvp("Scenes", sceneFilePaths));

                for(auto& filePath : sceneFilePaths)
                {
                    m_SceneManager->AddFileToLoadList(filePath);
                }

                if(m_SceneManager->GetScenes().size() == 0 && sceneFilePaths.size() == sceneIndex)
                {
                    m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                    m_SceneManager->SwitchScene(0);
                }
            }
            if(projectVersion > 3)
            {
                archive(cereal::make_nvp("SceneIndex", sceneIndex));
                m_SceneManager->SwitchScene(sceneIndex);
            }
            if(projectVersion > 4)
            {
                archive(cereal::make_nvp("Borderless", Borderless));
            }
        }

        const std::string& GetProjectRoot() const { return m_ProjectRoot; }

    protected:
        std::string m_ProjectRoot;
        std::string m_ProjectName;

    private:
        bool OnWindowClose(WindowCloseEvent& e);

        //Start proj saving
        uint32_t Width, Height;
        bool Fullscreen;
        bool VSync;
        bool Borderless = false;
        bool ShowConsole = true;
        std::string Title;
        int RenderAPI;
        //

        uint32_t m_Frames = 0;
        uint32_t m_Updates = 0;
        float m_SecondTimer = 0.0f;
        bool m_Minimized = false;
        bool m_SceneActive = true;

        uint32_t m_SceneViewWidth = 0;
        uint32_t m_SceneViewHeight = 0;
        bool m_SceneViewSizeUpdated = false;

        UniqueRef<Window> m_Window;
        UniqueRef<SceneManager> m_SceneManager;
        UniqueRef<SystemManager> m_SystemManager;
        UniqueRef<Graphics::RenderGraph> m_RenderGraph;
        UniqueRef<ImGuiManager> m_ImGuiManager;
        UniqueRef<Timer> m_Timer;
        SharedRef<ShaderLibrary> m_ShaderLibrary;

        AppState m_CurrentState = AppState::Loading;
        EditorState m_EditorState = EditorState::Preview;
        AppType m_AppType = AppType::Editor;

        static Application* s_Instance;

        NONCOPYABLE(Application)
    };

    //Defined by client
    Application* CreateApplication();
}
