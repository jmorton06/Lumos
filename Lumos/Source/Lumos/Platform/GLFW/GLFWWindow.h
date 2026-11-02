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
        void Show() override;
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

        inline float GetContentScale() const override
        {
            return m_Data.ContentScale;
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
        void RefreshSize() override;

        void SetWindowSize(uint32_t width, uint32_t height) override;
        Vec2 GetWindowSize() const override;
        void SetWindowPosition(int x, int y) override;
        Vec2 GetWindowPosition() const override;
        Vec2 GetCursorScreenPosition() const override;
        void SetDecorated(bool decorated) override;
        void SetFloating(bool floating) override;
        void SetWindowShape(float cornerRadius, bool shadow) override;
        void SetTransparent(bool transparent) override;
        void Minimise() override;
        TDArray<std::string> PopDroppedFiles() override;
        float GetMonitorXScale();

        static void MakeDefault();

    protected:
        static Window* CreateFuncGLFW();

        GLFWwindow* m_Handle;

        struct WindowData
        {
            std::string Title;
            TDArray<std::string> DroppedFiles;
            uint32_t Width, Height;
            bool VSync;
            bool Exit;
            Graphics::RenderAPI m_RenderAPI;
            float DPIScale;
            float ContentScale; // UI scale - see Window::GetContentScale

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}
