#pragma once
// Concrete DirectX 12 implementation of IRenderer.
// This prototype implements:
//   - Debug layer (Debug builds)
//   - DXGI factory + adapter enumeration
//   - D3D12 device
//   - Command queue, allocator, list
//   - Swap chain (double or triple buffered)
//   - RTV descriptor heap + render target views
//   - DSV descriptor heap + depth-stencil buffer
//   - GPU-CPU fence synchronization
//   - Per-frame clear to configurable color
//
// Not implemented (intentional prototype scope):
//   - PSO, root signature, vertex buffers (belong to game-side geometry)
//   - ImGui DX12 backend (integrate after this compiles clean)
//   - MSAA (add later by setting SampleDesc in PSO)

#include "Engine/Renderer/IRenderer.h"
#include "Engine/Core.h"


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace UwU_Engine
{

    using Microsoft::WRL::ComPtr;
    //using namespace DirectX;
    //using namespace DirectX::PackedVector;

    class UWU_API DX12Renderer : public IRenderer
    {
    public:
        DX12Renderer() = default;
        ~DX12Renderer() override { Shutdown(); }

        // IRenderer interface
        bool Init(void* windowHandle, uint32_t width, uint32_t height,
            const RendererConfig& cfg = {}) override;
        void Shutdown()   override;
        void OnResize(uint32_t width, uint32_t height) override;

        void BeginFrame() override;
        void EndFrame()   override;

        uint32_t GetWidth()  const override { return m_width; }
        uint32_t GetHeight() const override { return m_height; }
        bool     IsReady()   const override { return m_ready; }

        void SetClearColor(float r, float g, float b, float a = 1.0f) override
        {
            m_clearColor.f[0] = r;
            m_clearColor.f[1] = g;
            m_clearColor.f[2] = b;
            m_clearColor.f[3] = a;
        }

        void SetClearColor(DirectX::XMVECTORF32 color) {m_clearColor = color;}

        // Expose raw device for game-side resource creation (geometry, textures…)
        // Returns nullptr if not yet initialized.
        ID3D12Device* GetDevice()      const { return m_device.Get(); }
        ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }

    private:
        //Init helpers
        bool CreateDebugLayer();
        bool CreateFactory();
        bool CreateDevice();
        bool CreateCommandObjects();   // queue + allocator + list
        bool CreateSwapChain(HWND hwnd);
        bool CreateRTVHeap();
        bool CreateDSVHeap();
        bool CreateRenderTargetViews();
        bool CreateDepthStencilBuffer();

        //Sync helpers
        void FlushCommandQueue();      // CPU blocks until GPU finishes
        void WaitForPreviousFrame();

        //Descriptor helpers
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferRTV() const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()     const;
        ID3D12Resource* CurrentBackBuffer()     const;

    private:
        static constexpr uint32_t kSwapChainBuffers = 2;

        HWND                    m_hwnd = nullptr;
        uint32_t                m_width = 0;
        uint32_t                m_height = 0;
        bool                    m_ready = false; 
        DirectX::XMVECTORF32 m_clearColor = { DirectX::Colors::Black };// { 0.1f, 0.1f, 0.15f, 1.0f };

        DXGI_FORMAT m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT m_depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ComPtr<IDXGIFactory6>               m_factory;
        ComPtr<ID3D12Device>                m_device;
        ComPtr<ID3D12CommandQueue>          m_commandQueue;
        ComPtr<ID3D12CommandAllocator>      m_commandAllocator;
        ComPtr<ID3D12GraphicsCommandList>   m_commandList;
        ComPtr<IDXGISwapChain3>             m_swapChain;

        ComPtr<ID3D12DescriptorHeap>        m_rtvHeap;
        ComPtr<ID3D12DescriptorHeap>        m_dsvHeap;
        UINT m_rtvDescriptorSize = 0;
        UINT m_dsvDescriptorSize = 0;

        std::array<ComPtr<ID3D12Resource>, kSwapChainBuffers> m_renderTargets;
        ComPtr<ID3D12Resource>              m_depthStencilBuffer;

        // Fence for CPU-GPU sync
        ComPtr<ID3D12Fence>                 m_fence;
        uint64_t                            m_fenceValue = 0;
        HANDLE                              m_fenceEvent = nullptr;

        uint32_t                            m_backBufferIndex = 0;
    };

}
