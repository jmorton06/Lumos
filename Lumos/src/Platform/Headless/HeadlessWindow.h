#pragma once
#include "lmpch.h"
#include "Core/OS/Window.h"

namespace Lumos
{
	class LUMOS_EXPORT HeadlessWindow : public Window
	{
	public:
		HeadlessWindow(const WindowProperties& properties);
		~HeadlessWindow();

		void ToggleVSync() override;
		void SetVSync(bool set) override;
		void SetWindowTitle(const String& title) override;
		void SetBorderlessWindow(bool borderless) override;
		void OnUpdate() override;
		void HideMouse(bool hide) override;
		void SetMousePosition(const Maths::Vector2& pos) override;
        void UpdateCursorImGui() override;

		bool Init(const WindowProperties& properties);

		_FORCE_INLINE_ void* GetHandle() override { return nullptr; }

		_FORCE_INLINE_ String GetTitle() const override { return m_Data.Title; }
		_FORCE_INLINE_ u32 GetWidth()  const override { return m_Data.Width; }
		_FORCE_INLINE_ u32 GetHeight() const override { return m_Data.Height; }
		_FORCE_INLINE_ float GetScreenRatio() const override { return (float)m_Data.Width / (float)m_Data.Height; }
		_FORCE_INLINE_ bool GetExit() const override { return m_Data.Exit; }
		_FORCE_INLINE_ void SetExit(bool exit) override { m_Data.Exit = exit; }
		_FORCE_INLINE_ void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }

		void SetIcon(const String& file, const String& smallIconFilePath = "") override;

	     static void MakeDefault();

    protected:

		static Window* CreateFuncGLFW(const WindowProperties& properties);

		struct WindowData
		{
			std::string Title;
			u32 Width, Height;
			bool VSync;
			bool Exit;
			Graphics::RenderAPI m_RenderAPI;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};
}
