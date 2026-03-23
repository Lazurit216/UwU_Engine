#include "uwupch.h"
#include "Window.h"

#ifdef UWU_PLATFORM_WINDOWS
#include "Platforms/Windows/Win32Window.h"
#endif

namespace UwU_Engine
{
    std::unique_ptr<Window> Window::Create(const WindowProps& props)
    {
#ifdef UWU_PLATFORM_WINDOWS
        auto win = std::make_unique<Win32Window>();
        win->Init(props);
        return win;
#else
        UWU_ENGINE_ERROR("Unsupported platform");
        return nullptr;
#endif
    }
}