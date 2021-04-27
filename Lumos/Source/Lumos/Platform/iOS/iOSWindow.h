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

        inline uint32_t GetWidth() const override
        {
            return m_Data.Width;
        }
        inline uint32_t GetHeight() const override
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

        void SetIcon(const std::string& filePath, const std::string& smallIconFilePath) override {};
        void UpdateCursorImGui() override {};

        void OnKeyEvent(Lumos::InputCode::Key key, bool down);
        void OnTouchEvent(uint32_t xPos, uint32_t yPos, uint32_t count, bool down);
        void OnMouseMovedEvent(uint32_t xPos, uint32_t yPos);
        void OnResizeEvent(uint32_t width, uint32_t height);

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
            uint32_t Width, Height;
            bool VSync;
            bool Exit;
            //Graphics::RenderAPI m_RenderAPI;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
        void* m_Handle;
    };
}
