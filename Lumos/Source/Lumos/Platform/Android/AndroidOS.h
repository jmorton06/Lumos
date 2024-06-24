#pragma once
#ifndef LUMOS_PLATFORM_MACOS #include "Precompiled.h" #endif
#include "Core/OS/OS.h"

namespace Lumos
{
    class AndroidOS : public OS
    {
    public:
        AndroidOS();
        ~AndroidOS();

        void Init();
        void Run() override
        {
        }
        std::string GetExecutablePath() override;
        std::string GetAssetPath() override;
        void Vibrate() const override;

        void ShowKeyboard(bool open);
        bool HasWifiConnection();

        void OnFrame();
        void OnQuit();
        void OnKeyPressed(char keycode, bool down);
        void OnScreenPressed(uint32_t x, uint32_t y, uint32_t count, bool down);
        void OnMouseMovedEvent(uint32_t xPos, uint32_t yPos);
        void OnScreenResize(uint32_t width, uint32_t height);

        static void Alert(const char* message, const char* title);

        std::string GetModelName() const;

        void SetWindowSize(float x, float y)
        {
            m_X = x;
            m_Y = y;
        }
        float GetWidth()
        {
            return m_X;
        }
        float GetHeight()
        {
            return m_Y;
        }

        static AndroidOS* Get()
        {
            return (AndroidOS*)s_Instance;
        }

    private:
        float m_X, m_Y;
    };
}
