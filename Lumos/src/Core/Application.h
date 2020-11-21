#pragma once
#include "Core/Reference.h"
#include "Scene/SystemManager.h"
#include "Core/OS/Window.h"
#include "Scene/SceneManager.h"
#include "Utilities/AssetManager.h"

#include <cereal/types/vector.hpp>
#include <cereal/cereal.hpp>

#define LUMOS_EDITOR //temp

namespace Lumos
{
	class Timer;
	class Window;
	struct WindowProperties;
	class AudioManager;
	class SystemManager;
	class Editor;
	class LayerStack;
	class Layer;
	class ISystem;
	class Scene;
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;

	namespace Graphics
	{
		class RenderGraph;
		enum class RenderAPI : u32;
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

	public:
		Application(const std::string& projectRoot, const std::string& projectName);
		virtual ~Application();

		int Quit(bool pause = false, const std::string& reason = "");

		void Run();
		bool OnFrame();
		void OnUpdate(const TimeStep& dt);
		void OnRender();
		void OnEvent(Event& e);
		void OnImGui();
		void OnNewScene(Scene* scene);
		void OnExitScene();
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
		void ClearLayers();
		void OnSceneViewSizeUpdated(u32 width, u32 height);

		virtual void Init();

        LayerStack* GetSceneLayers() const
        {
            return m_LayerStack;
        }

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
        
        Ref<ShaderLibrary>& GetShaderLibrary() { return m_ShaderLibrary; }

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

		template<typename T>
		T* GetSystem()
		{
			return m_SystemManager->GetSystem<T>();
		}

		bool OnWindowResize(WindowResizeEvent& e);

		void SetSceneViewDimensions(u32 width, u32 height)
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

#ifdef LUMOS_EDITOR
		Editor* GetEditor() const
		{
			return m_Editor;
		};
#endif
    
        void EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName);
		
		virtual void Serialise(const std::string& filePath);
		virtual void Deserialise(const std::string& filePath);
		
		template<typename Archive>
			void save(Archive& archive) const
			
		{
            int projectVersion = 5;
			
			archive(cereal::make_nvp("Project Version", projectVersion));
			//Version 1
			archive(cereal::make_nvp("RenderAPI", RenderAPI),
						cereal::make_nvp("Width", m_Window->GetWidth()),
						cereal::make_nvp("Height", m_Window->GetHeight()),
						cereal::make_nvp("Fullscreen", Fullscreen),
						cereal::make_nvp("VSync", VSync),
						cereal::make_nvp("ShowConsole", ShowConsole),
						cereal::make_nvp("Title", Title),
						cereal::make_nvp("FilePath", FilePath)
						);
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
		
		
		template<typename Archive>
			void load(Archive& archive)
		{
			 int projectVersion = 0;
			archive(cereal::make_nvp("Project Version", projectVersion));
			
			if(projectVersion > 0)
			{
			archive(cereal::make_nvp("RenderAPI", RenderAPI),
					cereal::make_nvp("Width", Width),
					cereal::make_nvp("Height", Height),
					cereal::make_nvp("Fullscreen", Fullscreen),
					cereal::make_nvp("VSync", VSync),
					cereal::make_nvp("ShowConsole", ShowConsole),
					cereal::make_nvp("Title", Title),
							cereal::make_nvp("FilePath", FilePath));
			}
			if(projectVersion > 2)
			{
				std::vector<std::string> sceneFilePaths;
				archive(cereal::make_nvp("Scenes", sceneFilePaths));
				
				for(auto& filePath : sceneFilePaths)
				{
					m_SceneManager->AddFileToLoadList(filePath);
				}
			}
            if(projectVersion > 3)
            {
                int sceneIndex;
                archive(cereal::make_nvp("SceneIndex", sceneIndex));
                m_SceneManager->SwitchScene(sceneIndex);
            }
            if(projectVersion > 4)
            {
                archive(cereal::make_nvp("Borderless", Borderless));
            }

		}
					

	private:
		void PushLayerInternal(Layer* layer, bool overlay, bool sceneAdded);

		bool OnWindowClose(WindowCloseEvent& e);

		float m_UpdateTimer;
		UniqueRef<Timer> m_Timer;
		
		//Start proj saving
		u32 Width, Height;
		bool Fullscreen;
		bool VSync;
		bool Borderless = false;
		bool ShowConsole = true;
		std::string Title;
		int RenderAPI;
		std::string FilePath;
		//

		u32 m_Frames;
		u32 m_Updates;
		float m_SecondTimer = 0.0f;
		bool m_Minimized = false;
		bool m_SceneActive = true;

		u32 m_SceneViewWidth = 0;
		u32 m_SceneViewHeight = 0;
		bool m_SceneViewSizeUpdated = false;

		UniqueRef<Window> m_Window;
		UniqueRef<SceneManager> m_SceneManager;
		UniqueRef<SystemManager> m_SystemManager;
		UniqueRef<Graphics::RenderGraph> m_RenderGraph;
        
        Ref<ShaderLibrary> m_ShaderLibrary;

		LayerStack* m_LayerStack = nullptr;

		AppState m_CurrentState = AppState::Loading;
		EditorState m_EditorState = EditorState::Preview;
		AppType m_AppType = AppType::Editor;

		static Application* s_Instance;

		Layer* m_ImGuiLayer = nullptr;

#ifdef LUMOS_EDITOR
		Editor* m_Editor = nullptr;
#endif

		NONCOPYABLE(Application)
	};

	//Defined by client
	Application* CreateApplication();
}
