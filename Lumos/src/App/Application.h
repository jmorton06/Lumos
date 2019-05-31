#pragma once
#include "LM.h"

#define LUMOS_EDITOR //temp

namespace lumos
{
	class Timer;
	struct TimeStep;
	class Window;
	struct WindowProperties;
    class SceneManager;
	class AudioManager;
	class Entity;
	class Editor;
	class LayerStack;
	class Layer;
	class ISystem;
	class Scene;
	class Event;
	class WindowCloseEvent;
	class WindowResizeEvent;

	namespace graphics
	{
		class RenderManager;
		enum class RenderAPI;
	}

	namespace maths
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
		Application(const WindowProperties& properties);
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
		void OnNewScene(Scene* scene);
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
		void ClearLayers();;

		virtual void Init();

		LayerStack*						GetLayerStack()		const { return m_LayerStack; }
        SceneManager*					GetSceneManager()	const { return m_SceneManager.get(); }
		graphics::RenderManager*		GetRenderManager()	const { return m_RenderManager.get(); }
		AudioManager*					GetAudioManager()	const { return m_AudioManager.get(); }
        Window*							GetWindow()			const { return m_Window.get(); }
        AppState						GetState()			const { return m_CurrentState; }
		EditorState						GetEditorState()	const { return m_EditorState; }
		const std::vector<ISystem*>&	GetSystems()		const { return m_Systems; }

        void SetAppState(AppState state)		{ m_CurrentState = state; }
		void SetEditorState(EditorState state)	{ m_EditorState = state; }

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
		std::unique_ptr<AudioManager> m_AudioManager;
		std::unique_ptr<graphics::RenderManager> m_RenderManager;

		std::vector<ISystem*> m_Systems;

		LayerStack* m_LayerStack{};

        AppState m_CurrentState		= AppState::Loading;
		EditorState m_EditorState	= EditorState::Play;
		AppType m_AppType			= AppType::Editor;

		static Application* s_Instance;

#ifdef LUMOS_EDITOR
		Editor* m_Editor = nullptr;
#endif
	};

	//Defined by client
	Application* CreateApplication();
}
