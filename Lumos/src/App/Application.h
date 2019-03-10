#pragma once
#include "LM.h"
#include "Graphics/API/Context.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "Graphics/Layers/LayerStack.h"
#include "Events/Event.h"
#include <thread>

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

    enum class AppState
    {
        Running,
        Loading,
        Closing
    };

	enum class AppType
	{
		Game,
		Editor
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
		void OnGuizmo();
		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);
		void ClearLayers() { m_LayerStack->Clear(); };
        void SetScene(Scene* scene);

		virtual void Init();

        SceneManager* GetSceneManager() const { return m_SceneManager.get(); }
		RenderManager* GetRenderManager() const { return m_RenderManager.get(); }
		AudioManager* GetAudioManager() const { return m_AudioManager.get(); }
        Window* GetWindow() const { return m_Window.get(); }
        AppState GetState() const { return m_CurrentState; }
        void SetAppState(AppState state) { m_CurrentState = state; }

		maths::Vector2 GetWindowSize() const;

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
		float m_SecondTimer = 0.0f;

		std::unique_ptr<Window> m_Window;
        std::unique_ptr<SceneManager> m_SceneManager;
		std::unique_ptr<RenderManager> m_RenderManager;
		std::unique_ptr<AudioManager> m_AudioManager;

		LayerStack* m_LayerStack{};

        AppState m_CurrentState = AppState::Loading;
		AppType m_AppType = AppType::Editor;

		static Application* s_Instance;

		Entity* m_Selected = nullptr;

		bool m_FlipImGuiImage = false;

#ifdef LUMOS_EDITOR
		maths::Vector2 m_SceneViewSize;
#endif
	};

	//Defined by client
	Application* CreateApplication();
}
