#include "uwupch.h"
#include "InputManager.h"
#include "Engine/Events/KeyboardEvents.h"

namespace UwU_Engine
{
    void InputManager::BeginFrame()
    {
        m_prevMouseX = m_mouseX;
        m_prevMouseY = m_mouseY;
        m_mouseWheelDelta = 0.0f;
        m_keysPressed.clear();
        m_keysReleased.clear();
    }

    void InputManager::OnEvent(Event& e)
    {
        switch (e.GetType())
        {
        case EventType::KeyPressed:
        {
            const int keyCode = static_cast<KeyPressedEvent&>(e).GetKeyCode();
            if (m_keysDown.find(keyCode) == m_keysDown.end())
                m_keysPressed.insert(keyCode);
            m_keysDown.insert(keyCode);
            break;
        }
        case EventType::KeyReleased:
        {
            const int keyCode = static_cast<KeyReleasedEvent&>(e).GetKeyCode();
            m_keysDown.erase(keyCode);
            m_keysReleased.insert(keyCode);
            break;
        }
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
        case EventType::MouseWheel:
        {
            auto& me = static_cast<MouseWheelEvent&>(e);
            m_mouseWheelDelta += me.GetDelta();
            m_mouseX = me.GetX(); m_mouseY = me.GetY();
            break;
        }
        default: break;
        }
    }

    bool InputManager::IsKeyDown(int vkCode) const { return m_keysDown.count(vkCode) > 0; }

    bool InputManager::IsKeyPressed(int vkCode) const { return m_keysPressed.count(vkCode) > 0; }

    bool InputManager::IsKeyReleased(int vkCode) const { return m_keysReleased.count(vkCode) > 0; }

    void InputManager::BindAction(const std::string& actionName, std::initializer_list<int> keys)
    {
        m_actionBindings[actionName] = std::vector<int>(keys.begin(), keys.end());
    }

    bool InputManager::IsActionDown(const std::string& actionName) const
    {
        auto it = m_actionBindings.find(actionName);
        if (it == m_actionBindings.end())
            return false;

        for (int key : it->second)
            if (IsKeyDown(key))
                return true;
        return false;
    }

    bool InputManager::IsActionPressed(const std::string& actionName) const
    {
        auto it = m_actionBindings.find(actionName);
        if (it == m_actionBindings.end())
            return false;

        for (int key : it->second)
            if (IsKeyPressed(key))
                return true;
        return false;
    }

    bool InputManager::IsActionReleased(const std::string& actionName) const
    {
        auto it = m_actionBindings.find(actionName);
        if (it == m_actionBindings.end())
            return false;

        for (int key : it->second)
            if (IsKeyReleased(key))
                return true;
        return false;
    }

    bool InputManager::IsMouseDown(int button) const
    {
        return (button >= 0 && button < 3) ? m_mouseDown[button] : false;
    }

    bool InputManager::IsMouseDown(MouseButton btn) const
    {
        return IsMouseDown(static_cast<int>(btn));
    }


} 
