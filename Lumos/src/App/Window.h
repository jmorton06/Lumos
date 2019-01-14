#pragma once

#include "JM.h"
#include "Utilities/Timer.h"
#include "Maths/Vector2.h"
#include "System/Settings.h"
#include "Utilities/TSingleton.h"
#include "Events/Event.h"

namespace jm
{
	class Texture2D;

	struct JM_EXPORT WindowProperties
	{
		WindowProperties(System::CFG& cfg)
		{
			if (!cfg.FindUInt("width", Width))
				Width = 1280;
			if (!cfg.FindUInt("height", Height))
				Height = 720;
			if (!cfg.FindBool("fullscreen", Fullscreen))
				Fullscreen = false;
			if (!cfg.FindBool("vsync", VSync))
				VSync = true;
			if (!cfg.FindBool("borderless", Borderless))
				Borderless = false;
		}

		WindowProperties()
		{
			Width = 1280;
			Height = 720;
			Fullscreen = false;
			VSync = true;
			Borderless = false;
			Title = "Game Engine";
		}

		uint Width, Height;
		bool Fullscreen;
		bool VSync;
		bool Borderless;
		String Title;

	};

	class JM_EXPORT Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>; 

		static Window* Create(const std::string& title, const WindowProperties& properties);
		virtual ~Window() {};
		bool Initialise(const String& title, const WindowProperties& properties);

		inline Timer* GetWindowTimer() const { return m_Timer; }

		bool HasInitialised() const { return m_Init; };

		virtual bool GetExit() const = 0;
		virtual void SetExit(bool exit) = 0;

		inline void SetHasResized(bool resized){ m_HasResized = resized; }
		inline bool GetHasResized() const { return m_HasResized; }

		inline maths::Vector2 GetScreenSize() const { return m_ScreenSize; };

		virtual void ToggleVSync() = 0;
		virtual void SetVSync(bool set) = 0;
		virtual void SetWindowTitle(const String& title)  = 0;
		virtual void OnUpdate() = 0;
		virtual void SetBorderlessWindow(bool borderless) = 0;
		virtual void* GetHandle() { return nullptr; };
		virtual float GetScreenRatio() const = 0;
		virtual void HideMouse(bool hide) {};
		virtual void SetMousePosition(const maths::Vector2& pos) {};
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

		virtual uint GetWidth()  const = 0;
		virtual uint GetHeight() const = 0;

	protected:

		bool				m_Init;
		maths::Vector2		m_Position;
		maths::Vector2		m_ScreenSize;
		Timer*				m_Timer;
		bool				m_VSync;
		bool				m_HasResized;
	};

}