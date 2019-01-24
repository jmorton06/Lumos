#pragma once

#include "LM.h"
#include "App/Window.h"
#include "Graphics/API/Context.h"

struct GLFWwindow;

namespace Lumos
{

	class LUMOS_EXPORT GLFWWindow : public Window
	{
	public:
		GLFWWindow(const WindowProperties& properties, const String& title, RenderAPI api = RenderAPI::OPENGL);
		~GLFWWindow();

		void ToggleVSync() override;
		void SetVSync(bool set) override;
		void SetWindowTitle(const String& title) override;
		void SetBorderlessWindow(bool borderless) override;
		void OnUpdate() override;
		void HideMouse(bool hide) override;
		void SetMousePosition(const maths::Vector2& pos) override;

		bool Init(const WindowProperties& properties, const String& title);

		inline void* GetHandle() override { return m_Handle; }

		inline uint GetWidth()  const override { return m_Data.Width; }
		inline uint GetHeight() const override { return m_Data.Height; }
		inline float GetScreenRatio() const override { return (float)m_Data.Width / (float)m_Data.Height; }
		inline bool GetExit() const override { return m_Data.Exit; }
		inline void SetExit(bool exit) override { m_Data.Exit = exit; }
		inline void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetIcon(const String& file);

	protected:

		static int GLFWToJMKeyboardKey(int glfwKey);
		static int GLFWToJMMouseKey(int glfwKey);

		GLFWwindow* m_Handle;

		struct WindowData
		{
			std::string Title;
			uint Width, Height;
			bool VSync;
			bool Exit;
            RenderAPI m_RenderAPI;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}
