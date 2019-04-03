#pragma once
#include "LM.h"
#include "Graphics/API/Context.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#define LUMOS_EDITOR //temp

namespace Lumos
{
	class Timer;
	struct TimeStep;
	class Window;
    class SceneManager;
	class RenderManager;
	class AudioManager;
	class Entity;
	class Editor;
	class LayerStack;
	class Layer;
	class ISystem;
	enum class RenderAPI;

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
		Application(const WindowProperties& properties, const RenderAPI& api);
		virtual ~Application();

		Application(Application const&) = delete;
		Application& operator=(Application const&) = delete;

		int Quit(bool pause = false, const std::string &reason = "");

		void Run();
		bool OnFrame();
		void OnUpdate(TimeStep* dt);
		void OnRender();
		void OnEvent(Event& e);
		void OnImGui();
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
		void ClearLayers();;

		virtual void Init();

		LayerStack* GetLayerStack() const { return m_LayerStack; }
        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
		RenderManager* GetRenderManager() const { return m_RenderManager.get(); }
		AudioManager* GetAudioManager() const { return m_AudioManager.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }
        void SetAppState(AppState state) { m_CurrentState = state; }

		EditorState GetEditorState() const { return m_EditorState; }
		void SetEditorState(EditorState state) { m_EditorState = state; }

		maths::Vector2 GetWindowSize() const;

		static Application* Instance() { return s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

		float 	  				  	m_UpdateTimer;
		std::unique_ptr<Timer>	  	m_Timer;
		std::unique_ptr<TimeStep> 	m_TimeStep;

		uint m_Frames;
		uint m_Updates;
		float m_SecondTimer = 0.0f;

		std::unique_ptr<Window> m_Window;
        std::unique_ptr<SceneManager> m_SceneManager;
		std::unique_ptr<RenderManager> m_RenderManager;
		std::unique_ptr<AudioManager> m_AudioManager;

		std::vector<ISystem*> m_Systems;

		LayerStack* m_LayerStack{};

        AppState m_CurrentState = AppState::Loading;
		EditorState m_EditorState = EditorState::Play;
		AppType m_AppType = AppType::Editor;

		static Application* s_Instance;

#ifdef LUMOS_EDITOR
		Editor* m_Editor = nullptr;
#endif
	};

	//Defined by client
	Application* CreateApplication();
}
