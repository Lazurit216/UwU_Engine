#pragma once
#include "Engine/Core.h"
#include "Events.h"

namespace UwU_Engine
{
    enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

    class UWU_API MouseButtonPressedEvent : public Event
    {
    public:
        MouseButtonPressedEvent(MouseButton btn, float x, float y)
            : m_btn(btn), m_x(x), m_y(y) {
        }
        MouseButton GetButton() const { return m_btn; }
        float GetX() const { return m_x; }
        float GetY() const { return m_y; }
        EventType   GetType() const override { return EventType::MouseButtonPressed; }
        const char* GetName() const override { return "MouseButtonPressedEvent"; }
    private:
        MouseButton m_btn; float m_x, m_y;
    };

    class UWU_API MouseButtonReleasedEvent : public Event
    {
    public:
        MouseButtonReleasedEvent(MouseButton btn, float x, float y)
            : m_btn(btn), m_x(x), m_y(y) {
        }
        MouseButton GetButton() const { return m_btn; }
        float GetX() const { return m_x; }
        float GetY() const { return m_y; }
        EventType   GetType() const override { return EventType::MouseButtonReleased; }
        const char* GetName() const override { return "MouseButtonReleasedEvent"; }
    private:
        MouseButton m_btn; float m_x, m_y;
    };

    class UWU_API MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(float x, float y) : m_x(x), m_y(y) {}
        float GetX() const { return m_x; }
        float GetY() const { return m_y; }
        EventType   GetType() const override { return EventType::MouseMoved; }
        const char* GetName() const override { return "MouseMovedEvent"; }
    private:
        float m_x, m_y;
    };

    class UWU_API MouseWheelEvent : public Event
    {
    public:
        MouseWheelEvent(float delta, float x, float y)
            : m_delta(delta), m_x(x), m_y(y) {
        }
        float GetDelta() const { return m_delta; }
        float GetX() const { return m_x; }
        float GetY() const { return m_y; }
        EventType   GetType() const override { return EventType::MouseWheel; }
        const char* GetName() const override { return "MouseWheelEvent"; }
    private:
        float m_delta, m_x, m_y;
    };

}
