#pragma once
#include "lmpch.h"
#include "ECS/SystemManager.h"

#define LUMOS_EDITOR //temp

namespace Lumos
{
	class Timer;
	class Window;
	struct WindowProperties;
    class SceneManager;
	class AudioManager;
	class SystemManager;
	class Editor;
	class LayerStack;
	class Layer;
	class ISystem;
	class Scene;
	class Event;
	class Camera;
	class WindowCloseEvent;
	class WindowResizeEvent;

	namespace Graphics
	{
		class RenderManager;
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

    struct NameComponent
    {
        String name;
    };

	struct ActiveComponent
	{
		bool active;
	};

	class LUMOS_EXPORT Application
	{
		friend class Editor;
	public:
		Application(const WindowProperties& properties);
		virtual ~Application();

		int Quit(bool pause = false, const std::string &reason = "");

		void Run();
		bool OnFrame();
		void OnUpdate(TimeStep* dt);
		void OnRender();
		void OnEvent(Event& e);
		void OnImGui();
		void OnNewScene(Scene* scene);
		void OnExitScene();
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
		void ClearLayers();

		virtual void Init();

		LayerStack*					GetLayerStack()		const { return m_LayerStack; }
        SceneManager*				GetSceneManager()	const { return m_SceneManager.get(); }
		Graphics::RenderManager*	GetRenderManager()	const { return m_RenderManager.get(); }
        Window*						GetWindow()			const { return m_Window.get(); }
        AppState					GetState()			const { return m_CurrentState; }
		EditorState					GetEditorState()	const { return m_EditorState; }
		Camera*						GetActiveCamera()	const { return m_ActiveCamera; }
		SystemManager*				GetSystemManager()	const { return m_SystemManager.get(); }

        void SetAppState(AppState state)		{ m_CurrentState = state; }
		void SetEditorState(EditorState state)	{ m_EditorState = state; }
		void SetActiveCamera(Camera* camera);

		void SetSceneActive(bool active) { m_SceneActive = active; }
		bool GetSceneActive() const { return m_SceneActive; }
		Maths::Vector2 GetWindowSize() const;

		static Application* Instance() { return s_Instance; }
		static void Release() { delete s_Instance; s_Instance = nullptr; }

		template<typename T>
		T* GetSystem()
		{
			return m_SystemManager->GetSystem<T>();
		}

	private:

		void PushLayerInternal(Layer* layer, bool overlay, bool sceneAdded);

		bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

		float m_UpdateTimer;
		Scope<Timer> m_Timer;

		u32 m_Frames;
		u32 m_Updates;
		float m_SecondTimer = 0.0f;
		bool m_Minimized = false;
		bool m_SceneActive = true;

		Scope<Window> m_Window;
        Scope<SceneManager> m_SceneManager;
		Scope<SystemManager> m_SystemManager;
		Scope<Graphics::RenderManager> m_RenderManager;

		Camera* m_ActiveCamera = nullptr;

		LayerStack* m_LayerStack{};
		std::vector<Layer*> m_CurrentSceneLayers;

        AppState m_CurrentState		= AppState::Loading;
		EditorState m_EditorState	= EditorState::Play;
		AppType m_AppType			= AppType::Editor;

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
