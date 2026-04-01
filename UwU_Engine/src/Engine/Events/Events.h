#pragma once
#include "Engine/Core.h"

namespace UwU_Engine
{
    enum class EventType
    {
        None = 0,
        WindowClose,
        WindowMinimize,
        WindowMaximize,
        WindowRestore,
        WindowResize
    };

    class UWU_API Event
    {
    public:
        virtual ~Event() = default;
        virtual EventType GetType() const = 0;
        virtual const char* GetName() const = 0;
        bool Handled = false;
    };

    // Usage:
    //   EventDispatcher d(event);
    //   d.Dispatch<WindowResizeEvent>(
    //       std::bind(&Application::OnWindowResize, this, std::placeholders::_1), EventType::WindowResize);
    class EventDispatcher
    {
    public:
        explicit EventDispatcher(Event& event) : m_event(event) {}

        template<typename T>
        void Dispatch(std::function<void(T&)> fn, EventType type)
        {
            if (m_event.GetType() == type && !m_event.Handled)
            {
                fn(static_cast<T&>(m_event));
                m_event.Handled = true;
            }
        }
    private:
        Event& m_event;
    };
}