#pragma once

#include "Event.h"

#include <sstream>

namespace Lumos
{
    class LUMOS_EXPORT WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height)
            : m_Width(width)
            , m_Height(height)
            , m_DPIScale(1.0f)
        {
        }

        WindowResizeEvent(uint32_t width, uint32_t height, float dpiScale)
            : m_Width(width)
            , m_Height(height)
            , m_DPIScale(dpiScale)
        {
        }

        inline uint32_t GetWidth() const { return m_Width; }
        inline uint32_t GetHeight() const { return m_Height; }

        inline float GetDPIScale() const { return m_DPIScale; }

        std::string ToString() const override
        {
            std::stringstream ss;
            ss << "WindowResizeEvent: " << m_Width << ", " << m_Height << ", " << m_DPIScale;
            return ss.str();
        }

        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)

    private:
        uint32_t m_Width, m_Height;

        float m_DPIScale;
    };

    class LUMOS_EXPORT WindowCloseEvent : public Event
    {
    public:
        WindowCloseEvent() { }

        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class LUMOS_EXPORT AppTickEvent : public Event
    {
    public:
        AppTickEvent() { }

        EVENT_CLASS_TYPE(AppTick)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class LUMOS_EXPORT AppUpdateEvent : public Event
    {
    public:
        AppUpdateEvent() { }

        EVENT_CLASS_TYPE(AppUpdate)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

    class LUMOS_EXPORT AppRenderEvent : public Event
    {
    public:
        AppRenderEvent() { }

        EVENT_CLASS_TYPE(AppRender)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };

}
