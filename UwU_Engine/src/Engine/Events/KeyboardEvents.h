#pragma once
#include "Engine/Core.h"
#include "Events.h"

namespace UwU_Engine
{
    class UWU_API KeyPressedEvent : public Event
    {
    public:
        explicit KeyPressedEvent(int keyCode) : m_keyCode(keyCode) {}
        int GetKeyCode() const { return m_keyCode; }

        EventType   GetType() const override { return EventType::KeyPressed; }
        const char* GetName() const override { return "KeyPressedEvent"; }
    private:
        int m_keyCode;
    };

    class UWU_API KeyReleasedEvent : public Event
    {
    public:
        explicit KeyReleasedEvent(int keyCode) : m_keyCode(keyCode) {}
        int GetKeyCode() const { return m_keyCode; }

        EventType   GetType() const override { return EventType::KeyReleased; }
        const char* GetName() const override { return "KeyReleasedEvent"; }
    private:
        int m_keyCode;
    };
}