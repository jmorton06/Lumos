#pragma once
#include "LM.h"
#include "Graphics/API/Context.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Graphics/Layers/LayerStack.h"
#include "Events/Event.h"
#include <thread>

namespace Lumos
{
	class Timer;
	struct TimeStep;
	class Window;
    class SceneManager;

    enum class AppState
    {
        Running,
        Loading,
        Closing
    };

	class LUMOS_EXPORT Application
	{
	public:
		Application(const WindowProperties& properties, const RenderAPI& api);
		virtual ~Application();

		int Quit(bool pause = false, const std::string &reason = "");

		void Run();
		bool OnFrame();
		void OnUpdate(TimeStep* dt);
		void OnRender();
		void OnEvent(Event& e);
		void OnImGui();
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
        void ClearLayers() { m_LayerStack.Clear(); };
        void SetScene(Scene* scene);

		virtual void Init();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
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
        std::unique_ptr<SceneManager> m_SceneManager;

		LayerStack m_LayerStack;

        AppState m_CurrentState = AppState::Loading;

		static Application* s_Instance;
	};

	//Defined by client
	Application* CreateApplication();
}
