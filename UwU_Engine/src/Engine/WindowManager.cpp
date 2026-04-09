#include "uwupch.h"
#include "WindowManager.h"
#include "Platforms/Windows/Win32Window.h"

namespace UwU_Engine
{
    Window* WindowManager::Create(const Window::WindowProps& props)
    {
        auto win = std::make_unique<Win32Window>();
        if (!win->Init(props))
        {
            UWU_ENGINE_ERROR("[WindowManager] Failed to create window '{}'", props.title);
            return nullptr;
        }
        win->Show();
        Window* raw = win.get();
        m_windows.push_back(std::move(win));
        UWU_ENGINE_INFO("[WindowManager] Window '{}' created", props.title);
        return raw;
    }

    void WindowManager::PollAll()
    {
        for (auto& w : m_windows)
            if (!w->ShouldClose())
                w->PollEvents();
    }
}
