#pragma once

#include "Engine/Core.h"
#include "Engine/Events/Events.h"
#include "Engine/Events/MouseEvents.h"

namespace UwU_Engine
{
    class UWU_API InputManager
    {
    public:
        // Feed every engine event - ignores types it doesn't own.
        void OnEvent(Event& e);

        // Returns true while the key is physically held.
        // Use Windows VK codes: VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_ESCAPE, etc.
        bool IsKeyDown(int vkCode) const;
        bool IsKeyPressed(int vkCode) const;
        bool IsKeyReleased(int vkCode) const;

        void BindAction(const std::string& actionName, std::initializer_list<int> keys);
        bool IsActionDown(const std::string& actionName) const;
        bool IsActionPressed(const std::string& actionName) const;
        bool IsActionReleased(const std::string& actionName) const;

        // button: 0 = Left, 1 = Right, 2 = Middle
        bool IsMouseDown(int button)       const;
        bool IsMouseDown(MouseButton btn)  const;

        float GetMouseX() const { return m_mouseX; }
        float GetMouseY() const { return m_mouseY; }

        void  BeginFrame();               // call once per frame before PollAll()
        float GetMouseDeltaX() const { return m_mouseX - m_prevMouseX; }
        float GetMouseDeltaY() const { return m_mouseY - m_prevMouseY; }
        float GetMouseWheelDelta() const { return m_mouseWheelDelta; }

    private:
        std::unordered_set<int> m_keysDown;
        std::unordered_set<int> m_keysPressed;
        std::unordered_set<int> m_keysReleased;
        std::unordered_map<std::string, std::vector<int>> m_actionBindings;
        bool  m_mouseDown[3] = { false, false, false };
        float m_mouseX = 0.f;
        float m_mouseY = 0.f;

        float m_prevMouseX = 0.f;
        float m_prevMouseY = 0.f;
        float m_mouseWheelDelta = 0.f;
    };

}
