#pragma once
#include "Event.h"
#include "Maths/Vector2.h"

namespace Lumos
{
    enum class GestureState
    {
        Began,
        Changed,
        Ended,
        Cancelled
    };

    enum class SwipeDirection
    {
        Left,
        Right,
        Up,
        Down
    };

    class LUMOS_EXPORT GesturePinchEvent : public Event
    {
    public:
        GesturePinchEvent(float scale, float velocity, const Vec2& center, GestureState state)
            : m_Scale(scale)
            , m_Velocity(velocity)
            , m_CenterPoint(center)
            , m_State(state)
        {
        }

        inline float GetScale() const { return m_Scale; }
        inline float GetVelocity() const { return m_Velocity; }
        inline const Vec2& GetCenterPoint() const { return m_CenterPoint; }
        inline GestureState GetState() const { return m_State; }

        EVENT_CLASS_TYPE(GesturePinch)
        EVENT_CLASS_CATEGORY(EventCategoryGesture | EventCategoryInput)

    private:
        float m_Scale;
        float m_Velocity;
        Vec2 m_CenterPoint;
        GestureState m_State;
    };

    class LUMOS_EXPORT GesturePanEvent : public Event
    {
    public:
        GesturePanEvent(const Vec2& translation, const Vec2& velocity, uint32_t numTouches, GestureState state)
            : m_Translation(translation)
            , m_Velocity(velocity)
            , m_NumTouches(numTouches)
            , m_State(state)
        {
        }

        inline const Vec2& GetTranslation() const { return m_Translation; }
        inline const Vec2& GetVelocity() const { return m_Velocity; }
        inline uint32_t GetNumTouches() const { return m_NumTouches; }
        inline GestureState GetState() const { return m_State; }

        EVENT_CLASS_TYPE(GesturePan)
        EVENT_CLASS_CATEGORY(EventCategoryGesture | EventCategoryInput)

    private:
        Vec2 m_Translation;
        Vec2 m_Velocity;
        uint32_t m_NumTouches;
        GestureState m_State;
    };

    class LUMOS_EXPORT GestureSwipeEvent : public Event
    {
    public:
        GestureSwipeEvent(SwipeDirection direction, uint32_t numTouches)
            : m_Direction(direction)
            , m_NumTouches(numTouches)
        {
        }

        inline SwipeDirection GetDirection() const { return m_Direction; }
        inline uint32_t GetNumTouches() const { return m_NumTouches; }

        EVENT_CLASS_TYPE(GestureSwipe)
        EVENT_CLASS_CATEGORY(EventCategoryGesture | EventCategoryInput)

    private:
        SwipeDirection m_Direction;
        uint32_t m_NumTouches;
    };

    class LUMOS_EXPORT GestureLongPressEvent : public Event
    {
    public:
        GestureLongPressEvent(const Vec2& location, GestureState state)
            : m_Location(location)
            , m_State(state)
        {
        }

        inline const Vec2& GetLocation() const { return m_Location; }
        inline GestureState GetState() const { return m_State; }

        EVENT_CLASS_TYPE(GestureLongPress)
        EVENT_CLASS_CATEGORY(EventCategoryGesture | EventCategoryInput)

    private:
        Vec2 m_Location;
        GestureState m_State;
    };
}
