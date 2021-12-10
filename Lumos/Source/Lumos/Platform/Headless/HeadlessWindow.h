#pragma once

#include "Core/OS/Window.h"

namespace Lumos
{
    class LUMOS_EXPORT HeadlessWindow : public Window
    {
    public:
        HeadlessWindow(const WindowDesc& properties);
        ~HeadlessWindow();

        void ToggleVSync() override;
        void SetVSync(bool set) override;
        void SetWindowTitle(const std::string& title) override;
        void SetBorderlessWindow(bool borderless) override;
        void OnUpdate() override;
        void HideMouse(bool hide) override;
        void SetMousePosition(const glm::vec2& pos) override;
        void UpdateCursorImGui() override;

        bool Init(const WindowDesc& properties);

        _FORCE_INLINE_ void* GetHandle() override
        {
            return nullptr;
        }

        _FORCE_INLINE_ std::string GetTitle() const override
        {
            return m_Data.Title;
        }
        _FORCE_INLINE_ uint32_t GetWidth() const override
        {
            return m_Data.Width;
        }
        _FORCE_INLINE_ uint32_t GetHeight() const override
        {
            return m_Data.Height;
        }
        _FORCE_INLINE_ float GetScreenRatio() const override
        {
            return (float)m_Data.Width / (float)m_Data.Height;
        }
        _FORCE_INLINE_ bool GetExit() const override
        {
            return m_Data.Exit;
        }
        _FORCE_INLINE_ void SetExit(bool exit) override
        {
            m_Data.Exit = exit;
        }
        _FORCE_INLINE_ void SetEventCallback(const EventCallbackFn& callback) override
        {
            m_Data.EventCallback = callback;
        }

        void SetIcon(const std::string& file, const std::string& smallIconFilePath = "") override;

        static void MakeDefault();

    protected:
        static Window* CreateFuncGLFW(const WindowDesc& properties);

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
            bool Exit;
            Graphics::RenderAPI m_RenderAPI;

            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}
