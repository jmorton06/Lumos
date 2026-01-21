#pragma once
#include "Precompiled.h"
#include "Core/OS/OS.h"
#include "Lumos/Events/GestureEvent.h"

#include <vulkan/vulkan_metal.h>

namespace Lumos
{
    class iOSOS : public OS
    {
    public:
        enum class iOSDeviceType
        {
            iPhone,
            iPad
        };

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

        void ShowKeyboard(bool bShow) override;
        bool HasWifiConnection();

        iOSDeviceType GetDeviceType() const;
        bool IsLandscape() const;

        bool OnFrame();
        void OnQuit();
        void OnKeyPressed(char keycode, bool down, bool cmd = false, bool shift = false, bool alt = false, bool ctrl = false);
        void OnScreenPressed(uint32_t x, uint32_t y, uint32_t count, bool down);
        void OnMouseMovedEvent(uint32_t xPos, uint32_t yPos);
        void OnScreenResize(uint32_t width, uint32_t height);

        void OnGesturePinch(float scale, float velocity, uint32_t x, uint32_t y, GestureState state);
        void OnGesturePan(float tx, float ty, float vx, float vy, uint32_t touches, GestureState state);
        void OnGestureSwipe(SwipeDirection direction, uint32_t touches);
        void OnGestureLongPress(uint32_t x, uint32_t y, GestureState state);

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
