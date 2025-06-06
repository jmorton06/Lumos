#pragma once
#include "Precompiled.h"
#include "Core/OS/OS.h"

#include <vulkan/vulkan_metal.h>

namespace Lumos
{
    class iOSOS : public OS
    {
    public:
        iOSOS();
        ~iOSOS();

        void Init();
        void Run() override
        {
        }
        std::string GetExecutablePath() override;
        std::string GetAssetPath() override;
        std::string GetCurrentWorkingDirectory() override;
        void Vibrate() const override;

        CAMetalLayer* GetLayerPtr() const
        {
            if(!m_LayerPtr)
            {
                LWARN("Invalid layer pointer passed to SetLayerPtr.");
            }
            return m_LayerPtr;
        }

        void SetLayerPtr(CAMetalLayer* layer)
        {
            if(!layer)
            {
                LWARN("Invalid layer pointer passed to SetLayerPtr.");
            }
            m_LayerPtr = layer;
        }

        void ShowKeyboard(bool open);
        bool HasWifiConnection();

        bool OnFrame();
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

        static iOSOS* Get();
        static CAMetalLayer* GetStaticLayer();

    private:
        CAMetalLayer* m_LayerPtr;
        float m_X, m_Y;
    };
}
