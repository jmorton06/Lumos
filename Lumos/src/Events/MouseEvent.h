#pragma once
#include "Event.h"

namespace Lumos
{

	class LUMOS_EXPORT MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y)
			: m_MouseX(x), m_MouseY(y) {}

		_FORCE_INLINE_ float GetX() const { return m_MouseX; }
		_FORCE_INLINE_ float GetY() const { return m_MouseY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: " << m_MouseX << ", " << m_MouseY;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};


	class LUMOS_EXPORT MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {}

		_FORCE_INLINE_ float GetXOffset() const { return m_XOffset; }
		_FORCE_INLINE_ float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset, m_YOffset;
	};

	class LUMOS_EXPORT MouseButtonEvent : public Event
	{
	public:
		_FORCE_INLINE_ int GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	protected:
		MouseButtonEvent(int button)
			:m_Button(button) {}

		int m_Button;
	};

	class LUMOS_EXPORT MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)

	};

	class LUMOS_EXPORT MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)

	};

	class LUMOS_EXPORT MouseEnterEvent : public Event
	{
	public:
		MouseEnterEvent(bool enter) : m_Entered(enter) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseEnterEvent: " << m_Entered;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseEntered)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

		_FORCE_INLINE_ bool GetEntered() const { return m_Entered; }

	protected:

		bool m_Entered;

	};


}
