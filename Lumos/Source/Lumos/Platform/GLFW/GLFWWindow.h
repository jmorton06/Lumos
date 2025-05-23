#pragma once

#include "Core/OS/Window.h"

struct GLFWwindow;

namespace Lumos
{
    namespace Graphics
    {
        enum class RenderAPI : uint32_t;
    }

    class LUMOS_EXPORT GLFWWindow : public Window
    {
    public:
        GLFWWindow();
        ~GLFWWindow();

        bool Init(const WindowDesc& properties) override;
        void ToggleVSync() override;
        void SetVSync(bool set) override;
        void SetWindowTitle(const std::string& title) override;
        void SetBorderlessWindow(bool borderless) override;
        void OnUpdate() override;
        void HideMouse(bool hide) override;
        void SetMousePosition(const Vec2& pos) override;
        void UpdateCursorImGui() override;
        void ProcessInput() override;
        void Maximise() override;
        void UpdateControllers() override;

        inline void* GetHandle() override
        {
            return m_Handle;
        }

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

        inline float GetDPIScale() const override
        {
            return m_Data.DPIScale;
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

        void SetIcon(const WindowDesc& desc) override;
        float GetMonitorXScale();

        static void MakeDefault();

    protected:
        static Window* CreateFuncGLFW();

        GLFWwindow* m_Handle;

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
            bool Exit;
            Graphics::RenderAPI m_RenderAPI;
            float DPIScale;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}
