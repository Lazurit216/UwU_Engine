#include "uwupch.h"
#include "Win32Window.h"
#include "Engine/Events/WindowEvents.h"
namespace UwU_Engine
{
    Win32Window::Win32Window() {
        m_hInstance = GetModuleHandle(nullptr);
        static int counter = 0;
        m_windowClassName = L"UwUWindow_" + std::to_wstring(++counter);
    }

    Win32Window::~Win32Window() {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    bool Win32Window::RegisterWindowClass() {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_hInstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = m_windowClassName.c_str();

        return RegisterClassExW(&wc) != 0;
    }

    HWND Win32Window::CreateWindowHandle(int width, int height, const std::wstring& title, bool fullscreen) {
        DWORD style = WS_OVERLAPPEDWINDOW;
        DWORD exStyle = 0;

        if (fullscreen) {
            style = WS_POPUP;
            exStyle = WS_EX_TOPMOST;
            width = GetSystemMetrics(SM_CXSCREEN);
            height = GetSystemMetrics(SM_CYSCREEN);
        }

        RECT rect = { 0, 0, width, height };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        int x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;

        return CreateWindowExW(
            exStyle,
            m_windowClassName.c_str(),
            title.c_str(),
            style,
            x, y,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr,
            m_hInstance,
            this   
        );
    }

    bool Win32Window::Init(const WindowProps& props) {
        m_width =props.width;
        m_height = props.height;
        m_title = props.title;

        if (!RegisterWindowClass()) {
            UWU_ENGINE_ERROR("Failed to register window class");
            //throw std::runtime_error("Failed to register window class");
        }

        std::wstring wtitle(props.title.begin(), props.title.end());

        m_hwnd = CreateWindowHandle(props.width, props.height, wtitle, props.fullscreen);
        if (!m_hwnd) {
            UWU_ENGINE_ERROR("Failed to create window");
            //throw std::runtime_error("Failed to create window");
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        return true;
    }

    void Win32Window::Show() {
        if (m_hwnd) ShowWindow(m_hwnd, SW_SHOW);
    }

    void Win32Window::Hide() {
        if (m_hwnd) ShowWindow(m_hwnd, SW_HIDE);
    }

    void Win32Window::Close() {
        m_shouldClose = true;
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    void Win32Window::PollEvents() {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void Win32Window::SwapBuffers() {
        // Здесь должен быть вызов SwapBuffers(hdc) если используется OpenGL
        // Для чистого WinAPI без контекста — можно оставить пустым
    }

    void Win32Window::SetTitle(const std::string& title) {
        m_title = title;
        std::wstring wtitle(title.begin(), title.end());
        if (m_hwnd) SetWindowTextW(m_hwnd, wtitle.c_str());
    }

    void Win32Window::Focus() {
        if (m_hwnd) SetForegroundWindow(m_hwnd);
    }

    bool Win32Window::IsFocused() const {
        return m_hwnd && (GetForegroundWindow() == m_hwnd);
    }

    void Win32Window::Minimize() {
        if (m_hwnd) ShowWindow(m_hwnd, SW_MINIMIZE);
    }

    void Win32Window::Maximize() {
        if (m_hwnd) ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    void Win32Window::Restore() {
        if (m_hwnd) ShowWindow(m_hwnd, SW_RESTORE);
    }

    // Статический обработчик сообщений
    LRESULT CALLBACK Win32Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
    {
        Win32Window* window = nullptr;

        if (msg == WM_NCCREATE) {
            auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            window = static_cast<Win32Window*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        }
        else {
            window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }

        if (window && window->m_eventCallback) 
        {
            switch (msg) 
            {
                case WM_CLOSE:
                {
                    WindowCloseEvent e;
                    window->m_eventCallback(e);
                    window->m_shouldClose = true;
                    DestroyWindow(hwnd);
                    return 0;
                }

                case WM_SIZE:
                {
                    int w = LOWORD(lParam);
                    int h = HIWORD(lParam);
                    window->m_width = w;
                    window->m_height = h;

                    switch (wParam)
                    {
                    case SIZE_MINIMIZED:
                    {
                        WindowMinimizeEvent e;
                        window->m_eventCallback(e);
                        break;
                    }
                    case SIZE_MAXIMIZED:
                    {
                        WindowMaximizeEvent e;
                        window->m_eventCallback(e);
                        // Also fire resize so renderer updates buffers
                        WindowResizeEvent re(w, h);
                        window->m_eventCallback(re);
                        break;
                    }
                    case SIZE_RESTORED:
                    {
                        WindowRestoreEvent e;
                        window->m_eventCallback(e);
                        // Also fire resize
                        WindowResizeEvent re(w, h);
                        window->m_eventCallback(re);
                        break;
                    }
                    }
                    return 0;
                }

                case WM_DESTROY:
                {
                    PostQuitMessage(0);
                    return 0;
                }
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}