#pragma once

#include "Event.h"

#include <sstream>

namespace Lumos
{
	class LUMOS_EXPORT WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(u32 width, u32 height)
			: m_Width(width), m_Height(height) {}

		_FORCE_INLINE_ u32 GetWidth() const { return m_Width; }
		_FORCE_INLINE_ u32 GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		u32 m_Width, m_Height;
	};

	class LUMOS_EXPORT WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() {}

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class LUMOS_EXPORT AppTickEvent : public Event
	{
	public:
		AppTickEvent() {}

		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class LUMOS_EXPORT AppUpdateEvent : public Event
	{
	public:
		AppUpdateEvent() {}

		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class LUMOS_EXPORT AppRenderEvent : public Event
	{
	public:
		AppRenderEvent() {}

		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

}
