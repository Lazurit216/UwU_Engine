#pragma once
#include "Core.h"
#include "Window.h"


namespace UwU_Engine
{
    class UWU_API WindowManager
    {
    public:
        WindowManager() = default;
        ~WindowManager() = default;

        // Creates, initialises, and shows a Win32Window.
        // Returns a raw pointer whose lifetime is managed by this manager.
        Window* Create(const Window::WindowProps& props);

        // Forward PollEvents to every live window.
        void PollAll();

        // Number of windows currently alive.
        std::size_t Count() const { return m_windows.size(); }

        // Raw access by index (for the main loop).
        Window* Get(std::size_t idx) const { return m_windows[idx].get(); }

    private:
        std::vector<std::unique_ptr<Window>> m_windows;

        WindowManager(const WindowManager&) = delete;
        WindowManager& operator=(const WindowManager&) = delete;
        WindowManager(WindowManager&&) noexcept = default;
        WindowManager& operator=(WindowManager&&) noexcept = default;
    };
}
