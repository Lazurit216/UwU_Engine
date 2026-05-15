#include "Win32Window.h"
#include "uwupch.h"
#include "Engine/Events/WindowEvents.h"
#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Events/MouseEvents.h"  

namespace UwU_Engine
{
    static std::wstring ToWide(const std::string& s)
    {
        if (s.empty()) return {};
        int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
        std::wstring r(n, 0);
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, r.data(), n);
        return r;
    }

    Win32Window::Win32Window()
    {
        m_hInstance = GetModuleHandleW(nullptr);
        static int s_count = 0;
        m_windowClassName = L"UwUWindow_" + std::to_wstring(++s_count);
    }

    Win32Window::~Win32Window()
    {
        if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
        UnregisterClassW(m_windowClassName.c_str(), m_hInstance);
    }

    bool Win32Window::Init(const WindowProps& props)
    {
        m_originalTitle = props.title;
        m_title = m_originalTitle;

        m_width = props.width;
        m_height = props.height;

        if (!RegisterWindowClass()) return false;
        m_hwnd = CreateWindowHandle(props.width, props.height,
            ToWide(props.title), props.fullscreen);
        if (!m_hwnd)
        {
            UWU_ENGINE_ERROR("[Win32Window] CreateWindowHandle failed: {}", GetLastError());
            return false;
        }
        UWU_ENGINE_INFO("[Win32Window] '{}' {}x{} created", m_title, m_width, m_height);
        return true;
    }

    bool Win32Window::RegisterWindowClass()
    {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconA(nullptr, IDI_APPLICATION);
        wc.lpszClassName = m_windowClassName.c_str();

        if (!RegisterClassExW(&wc))
        {
            UWU_ENGINE_ERROR("[Win32Window] RegisterClassEx failed: {}", GetLastError());
            return false;
        }
        return true;
    }

    HWND Win32Window::CreateWindowHandle(int w, int h,
        const std::wstring& title, bool fullscreen)
    {
        DWORD style = fullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
        DWORD exStyle = fullscreen ? WS_EX_TOPMOST : 0;

        if (fullscreen)
        {
            w = GetSystemMetrics(SM_CXSCREEN);
            h = GetSystemMetrics(SM_CYSCREEN);
        }

        RECT rect{ 0, 0, w, h };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        int x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;

        return CreateWindowExW(
            exStyle, m_windowClassName.c_str(), title.c_str(), style,
            x, y, rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, m_hInstance,
            this);  // passed to WM_NCCREATE via lpCreateParams
    }

    void Win32Window::Show() { if (m_hwnd) { ShowWindow(m_hwnd, SW_SHOW); UpdateWindow(m_hwnd); } }
    void Win32Window::Hide() { if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE); }
    void Win32Window::Minimize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MINIMIZE); }
    void Win32Window::Maximize() { if (m_hwnd) ShowWindow(m_hwnd, SW_MAXIMIZE); }
    void Win32Window::Restore() { if (m_hwnd) ShowWindow(m_hwnd, SW_RESTORE); }
    void Win32Window::Focus() { if (m_hwnd) SetForegroundWindow(m_hwnd); }
    bool Win32Window::IsFocused() const { return m_hwnd && GetForegroundWindow() == m_hwnd; }

    void Win32Window::Close()
    {
        m_shouldClose = true;
        if (m_hwnd) { DestroyWindow(m_hwnd); m_hwnd = nullptr; }
    }

    void Win32Window::PollEvents()
    {
        MSG msg;
        while (PeekMessageW(&msg, m_hwnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    void Win32Window::SwapBuffers()
    {

    }
    void Win32Window::SetTitle(const std::string& title)
    {
        m_title = title;
        if (m_hwnd) SetWindowTextW(m_hwnd, ToWide(title).c_str());
    }

    LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg,
        WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_NCCREATE)
        {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProcW(hwnd, msg, wParam, lParam);
        }

        auto* self = reinterpret_cast<Win32Window*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (!self || !self->m_eventCallback)
            return DefWindowProcW(hwnd, msg, wParam, lParam);

        switch (msg)
        {
        case WM_CLOSE:
        {
            self->m_shouldClose = true;
            WindowCloseEvent e;
            self->m_eventCallback(e);
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY:
            self->m_hwnd = nullptr;
            // Do NOT PostQuitMessage - Application controls the loop exit.
            return 0;

        case WM_SIZE:
        {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            self->m_width = w;
            self->m_height = h;

            switch (wParam)
            {
            case SIZE_MINIMIZED:
            {
                WindowMinimizeEvent e;
                self->m_eventCallback(e);
                break;
            }
            case SIZE_MAXIMIZED:
            {
                WindowMaximizeEvent me;
                self->m_eventCallback(me);
                if (w > 0 && h > 0)
                {
                    WindowResizeEvent re(w, h);
                    self->m_eventCallback(re);
                }
                break;
            }
            case SIZE_RESTORED:
            {
                WindowRestoreEvent re;
                self->m_eventCallback(re);
                if (w > 0 && h > 0)
                {
                    WindowResizeEvent rse(w, h);
                    self->m_eventCallback(rse);
                }
                break;
            }
            }
            return 0;
        }

        case WM_KEYDOWN:
        {
            KeyPressedEvent e(static_cast<int>(wParam));
            self->m_eventCallback(e);
            return 0;
        }

        case WM_KEYUP:
        {
            KeyReleasedEvent e(static_cast<int>(wParam));
            self->m_eventCallback(e);
            return 0;
        }

        case WM_LBUTTONDOWN:
        {
            SetCapture(hwnd);  // track mouse even when dragged outside window
            MouseButtonPressedEvent e(MouseButton::Left,
                static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
            self->m_eventCallback(e); return 0;
        }
        case WM_LBUTTONUP:
        {
            ReleaseCapture();
            MouseButtonReleasedEvent e(MouseButton::Left,
                static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
            self->m_eventCallback(e); return 0;
        }
        case WM_RBUTTONDOWN:
        {
            SetCapture(hwnd);
            MouseButtonPressedEvent e(MouseButton::Right,
                static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
            self->m_eventCallback(e); return 0;
        }
        case WM_RBUTTONUP:
        {
            ReleaseCapture();
            MouseButtonReleasedEvent e(MouseButton::Right,
                static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)));
            self->m_eventCallback(e); return 0;
        }

        case WM_MOUSEMOVE:
        {
            MouseMovedEvent e(static_cast<float>(LOWORD(lParam)),
                static_cast<float>(HIWORD(lParam)));
            self->m_eventCallback(e); return 0;
        }

        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
