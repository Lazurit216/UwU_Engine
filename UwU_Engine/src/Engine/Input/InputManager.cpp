#include "uwupch.h"
#include "InputManager.h"
#include "Engine/Events/KeyboardEvents.h"

namespace UwU_Engine
{
    void InputManager::BeginFrame()
    {
        m_prevMouseX = m_mouseX;
        m_prevMouseY = m_mouseY;
    }

    void InputManager::OnEvent(Event& e)
    {
        switch (e.GetType())
        {
        case EventType::KeyPressed:
            m_keysDown.insert(static_cast<KeyPressedEvent&>(e).GetKeyCode());
            break;
        case EventType::KeyReleased:
            m_keysDown.erase(static_cast<KeyReleasedEvent&>(e).GetKeyCode());
            break;
        case EventType::MouseButtonPressed:
        {
            auto& me = static_cast<MouseButtonPressedEvent&>(e);
            int idx = static_cast<int>(me.GetButton());
            if (idx >= 0 && idx < 3) m_mouseDown[idx] = true;
            m_mouseX = me.GetX(); m_mouseY = me.GetY();
            break;
        }
        case EventType::MouseButtonReleased:
        {
            int idx = static_cast<int>(static_cast<MouseButtonReleasedEvent&>(e).GetButton());
            if (idx >= 0 && idx < 3) m_mouseDown[idx] = false;
            break;
        }
        case EventType::MouseMoved:
        {
            auto& me = static_cast<MouseMovedEvent&>(e);
            m_mouseX = me.GetX(); m_mouseY = me.GetY();
            break;
        }
        default: break;
        }
    }

    bool InputManager::IsKeyDown(int vkCode) const { return m_keysDown.count(vkCode) > 0; }

    bool InputManager::IsMouseDown(int button) const
    {
        return (button >= 0 && button < 3) ? m_mouseDown[button] : false;
    }

    bool InputManager::IsMouseDown(MouseButton btn) const
    {
        return IsMouseDown(static_cast<int>(btn));
    }


} 