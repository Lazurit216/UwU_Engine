#pragma once
// Adapter interface for all rendering backends.
//
// Lifecycle contract:
//   Init(hwnd, w, h)  -  one call at startup
//   BeginFrame()      -  start of each frame (transition resources, clear)
//   EndFrame()        -  end of each frame (present, flip)
//   OnResize(w, h)    -  whenever the window changes size
//   Shutdown()        -  release all GPU resources (call before destructor)

#include "Engine/Core.h"
namespace UwU_Engine
{

    struct RendererConfig
    {
        uint32_t    backBufferCount = 2;  
        bool        vsync = true;
        bool        debugLayer = true; // enable DX12 debug layer in Debug builds
        std::string windowTitle = "Renderer";
    };

    class UWU_API IRenderer
    {
    public:
        virtual ~IRenderer() = default;

        //Lifecycle

        virtual bool Init(void* windowHandle, uint32_t width, uint32_t height,
            const RendererConfig& cfg = {}) = 0;

        static std::unique_ptr<IRenderer> Create();

        virtual void Shutdown() = 0;
        virtual void OnResize(uint32_t width, uint32_t height) = 0;

        //Per-frame
        virtual void BeginFrame() = 0;
        virtual void EndFrame() = 0;

        //Utility
        virtual uint32_t GetWidth()  const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual bool     IsReady()   const = 0;

        // Clear colour that states can set (e.g. menus use a different colour).
        virtual void SetClearColor(float r, float g, float b, float a = 1.0f) = 0;
    };

}
