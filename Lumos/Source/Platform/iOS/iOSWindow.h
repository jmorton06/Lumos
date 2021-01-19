#pragma once
#include "Core/OS/Window.h"
#include "Core/OS/KeyCodes.h"

namespace Lumos
{
	class iOSWindow : public Window
	{
	public:
		iOSWindow(const WindowProperties& properties);
		~iOSWindow();

		void ToggleVSync() override;
		void SetWindowTitle(const std::string& title) override;
		void SetBorderlessWindow(bool borderless) override;
		void OnUpdate() override;

		bool Init(const WindowProperties& properties, const std::string& title);

		void* GetHandle() override
		{
			return (void*)m_Handle;
		}

		inline u32 GetWidth() const override
		{
			return m_Data.Width;
		}
		inline u32 GetHeight() const override
		{
			return m_Data.Height;
		}

		bool GetExit() const override
		{
			return false;
		}
		
		void SetExit(bool exit) override
		{
		}

		void SetVSync(bool set) override
		{
		}
		
		float GetScreenRatio() const override
		{
			return (float)m_Data.Width / (float)m_Data.Height;
		}

		void SetEventCallback(const EventCallbackFn& callback) override
		{
			m_Data.EventCallback = callback;
		};
		
		void SetIcon(const std::string& filePath, const std::string& smallIconFilePath) override{};
		void UpdateCursorImGui() override{};

		void OnKeyEvent(Lumos::InputCode::Key key, bool down);
		void OnTouchEvent(u32 xPos, u32 yPos, u32 count, bool down);
		void OnMouseMovedEvent(u32 xPos, u32 yPos);
		void OnResizeEvent(u32 width, u32 height);

		std::string GetTitle() const override
		{
			return "Mobile";
		}

		static void MakeDefault();

	protected:
		static Window* CreateFunciOS(const WindowProperties& properties);

		struct WindowData
		{
			std::string Title;
			u32 Width, Height;
			bool VSync;
			bool Exit;
			//Graphics::RenderAPI m_RenderAPI;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
		void* m_Handle;
	};
}
