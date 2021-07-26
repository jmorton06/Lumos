#pragma once

#include "Core/OS/Window.h"
#include <Windows.h>

namespace Lumos
{
    namespace Graphics
    {
        enum class RenderAPI : uint32_t;
    }

    class LUMOS_EXPORT WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowDesc& properties);
        ~WindowsWindow();

        void ToggleVSync() override;
        void SetWindowTitle(const std::string& title) override;
        void SetBorderlessWindow(bool borderless) override;
        void OnUpdate() override;
        void ProcessInput() override;

        void SetVSync(bool set) override {};
        void HideMouse(bool hide) override {};
        void SetMousePosition(const Maths::Vector2& pos) override {};
        void UpdateCursorImGui() override;
        void SetIcon(const std::string& filePath, const std::string& smallIconFilePath = "") override;

        bool Init(const WindowDesc& properties);

        inline std::string GetTitle() const override
        {
            return m_Data.Title;
        }
        inline uint32_t GetWidth() const override
        {
            return m_Data.Width;
        }
        inline uint32_t GetHeight() const override
        {
            return m_Data.Height;
        }
        inline float GetScreenRatio() const override
        {
            return (float)m_Data.Width / (float)m_Data.Height;
        }
        inline bool GetExit() const override
        {
            return m_Data.Exit;
        }
        inline void SetExit(bool exit) override
        {
            m_Data.Exit = exit;
        }
        inline void SetEventCallback(const EventCallbackFn& callback) override
        {
            m_Data.EventCallback = callback;
        }

        inline void* GetHandle() override
        {
            return hWnd;
        }

        struct WindowData
        {
            std::string Title;
            uint32_t Width = 0, Height = 0;
            bool VSync;
            bool Exit;
            Graphics::RenderAPI m_RenderAPI;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;

        HINSTANCE GetHInstance() const
        {
            return hInstance;
        }
        HWND GetHWND() const
        {
            return hWnd;
        }

        static void MakeDefault();

    protected:
        static Window* CreateFuncWindows(const WindowDesc& properties);

        HINSTANCE hInstance {};
        HDC hDc {};
        HWND hWnd;
        RAWINPUTDEVICE rid {};

        HICON m_BigIcon;
        HICON m_SmallIcon;
    };

}