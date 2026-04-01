#pragma once
#include "Engine/Core.h"
#include "Events.h"

namespace UwU_Engine
{
    class UWU_API WindowCloseEvent : public Event
    {
    public:
        EventType   GetType() const override { return EventType::WindowClose; }
        const char* GetName() const override { return "WindowCloseEvent"; }
    };

    class UWU_API WindowMinimizeEvent : public Event
    {
    public:
        EventType   GetType() const override { return EventType::WindowMinimize; }
        const char* GetName() const override { return "WindowMinimizeEvent"; }
    };

    class UWU_API WindowMaximizeEvent : public Event
    {
    public:
        EventType   GetType() const override { return EventType::WindowMaximize; }
        const char* GetName() const override { return "WindowMaximizeEvent"; }
    };

    class UWU_API WindowRestoreEvent : public Event
    {
    public:
        EventType   GetType() const override { return EventType::WindowRestore; }
        const char* GetName() const override { return "WindowRestoreEvent"; }
    };

    class UWU_API WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(uint32_t w, uint32_t h) : m_width(w), m_height(h) {}
        uint32_t GetWidth()  const { return m_width; }
        uint32_t GetHeight() const { return m_height; }
        EventType   GetType() const override { return EventType::WindowResize; }
        const char* GetName() const override { return "WindowResizeEvent"; }
    private:
        uint32_t m_width, m_height;
    };
}