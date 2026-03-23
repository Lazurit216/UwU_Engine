#pragma once
#include "Core.h"

namespace UwU_Engine
{
    class UWU_API Window
    {
    public:
        Window() = default;
        virtual ~Window() = default;

        // Запрещаем копирование и перемещение (окно — уникальный ресурс)
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        struct WindowProps
        {
            int         width = 1280;
            int         height = 720;
            std::string title = "UwU Engine";
            bool        fullscreen = false;
            bool        resizable = true;
            bool        vsync = true;
        };

        virtual bool Init(const WindowProps& props) = 0;

        static std::unique_ptr<Window> Create(const WindowProps& props);
        
        //virtual bool Create(const WindowProps& props) = 0;

        virtual void Show() = 0;
        virtual void Hide() = 0;
        virtual void Close() = 0;
        virtual bool ShouldClose() const = 0;
        virtual void PollEvents() = 0;
        virtual void SwapBuffers() = 0;

        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;

        virtual void* GetNativeHandle() const = 0;

        virtual void SetTitle(const std::string& title) = 0;

        virtual void Focus() = 0;
        virtual bool IsFocused() const = 0;

        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual void Restore() = 0;

        using ResizeCallback = std::function<void(int w, int h)>;
        void SetResizeCallback(ResizeCallback cb) { m_onResize = cb; }

    protected:
        bool m_shouldClose = false;
        int m_width = 0;
        int m_height = 0;
        std::string m_title;
        ResizeCallback m_onResize;
    };
}