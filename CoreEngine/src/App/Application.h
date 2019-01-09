#pragma once
#include "JM.h"
#include "Graphics/API/Context.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include <thread>

namespace jm
{
	class Timer;
	struct TimeStep;
	class Window;
    class GraphicsPipeline;
    class SceneManager;

    enum class AppState
    {
        Running,
        Loading,
        Closing
    };

	class JM_EXPORT Application
	{
	public:
		Application(const WindowProperties& properties, const RenderAPI& api);
		virtual ~Application();

		int Quit(bool pause = false, const std::string &reason = "");

		void Run();
		bool OnFrame();
		void OnEvent(Event& e);
		virtual void Init();

		void DefaultControls();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
        GraphicsPipeline* GetGraphicsPipeline() const { return m_GraphicsPipeline.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }

		static void PhysicsUpdate(float targetUpdateTime);

		static Application* Instance() { return s_Instance; }

	private:
		bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

		float 	  				  	m_UpdateTimer;
		std::unique_ptr<Timer>	  	m_Timer;
		std::unique_ptr<TimeStep> 	m_TimeStep;
		std::thread 				m_PhysicsThread;

		uint m_Frames;
		uint m_Updates;
		float m_SecondTimer;

		std::unique_ptr<Window> m_Window;
        std::unique_ptr<GraphicsPipeline> m_GraphicsPipeline;
        std::unique_ptr<SceneManager> m_SceneManager;

        AppState m_CurrentState = AppState::Loading;

		static Application* s_Instance;
	};

	//Defined by client
	Application* CreateApplication();
}
