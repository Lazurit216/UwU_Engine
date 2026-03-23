#pragma once
#include "Engine/Core.h"
#include "Engine/Window.h"

namespace UwU_Engine
{
    class UWU_API Win32Window final : public Window {
    public:
        Win32Window();
        ~Win32Window() override;

        bool Init(const WindowProps& props) override;

        void Show() override;
        void Hide() override;
        void Close() override;
        bool ShouldClose() const override { return m_shouldClose; }
        void PollEvents() override;
        void SwapBuffers() override; 

        int GetWidth() const override { return m_width; }
        int GetHeight() const override { return m_height; }

        void* GetNativeHandle() const override { return (void*)m_hwnd; }

        void SetTitle(const std::string& title) override;

        void Focus() override;
        bool IsFocused() const override;

        void Minimize() override;
        void Maximize() override;
        void Restore() override;

    private:
        HWND m_hwnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        std::wstring m_windowClassName;

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        bool RegisterWindowClass();
        HWND CreateWindowHandle(int width, int height, const std::wstring& title, bool fullscreen);
    };
}