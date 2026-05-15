// Engine/Renderer/DirectX12/DX12Renderer.cpp
#include "uwupch.h"
#include "DX12Renderer.h"
#include "d3dx12.h"
#include "Engine/Assets/DX12Apps/DX12Triangle.h"
#include "Engine/Renderer/IDrawable.h"

#define CHECK_HR(expr)                                                      \
    do {                                                                    \
        HRESULT _hr = (expr);                                               \
        if (FAILED(_hr)) {                                                  \
            UWU_ENGINE_ERROR("[DX12] {} failed, hr={:#x}",                 \
                             #expr, (unsigned)_hr);                         \
            return false;                                                   \
        }                                                                   \
    } while(0)

namespace UwU_Engine
{
    static std::string NarrowAscii(const std::wstring& value)
    {
        std::string result;
        result.reserve(value.size());
        for (wchar_t ch : value)
            result.push_back(ch >= 0 && ch <= 0x7f ? static_cast<char>(ch) : '?');
        return result;
    }

    // Public

    bool DX12Renderer::Init(void* windowHandle, uint32_t width, uint32_t height,
        const RendererConfig& cfg)
    {
        m_hwnd = static_cast<HWND>(windowHandle);
        m_width = width;
        m_height = height;
        m_vsync = cfg.vsync;

        UWU_ENGINE_INFO("[DX12] Init {}x{}", width, height);

#ifdef _DEBUG
        if (cfg.debugLayer)
            if (!CreateDebugLayer())
                UWU_ENGINE_WARN("[DX12] Debug layer unavailable");
#endif

        if (!CreateFactory()) return false;
        if (!CreateDevice())  return false;

        m_ownsDevice = true;
        return InitSwapChainAndResources(m_hwnd, m_vsync);
    }

    bool DX12Renderer::InitShared(void* windowHandle, uint32_t width, uint32_t height,
        ID3D12Device* device, IDXGIFactory6* factory,
        bool vsync)
    {
        if (!device || !factory)
        {
            UWU_ENGINE_ERROR("[DX12] InitShared: null device or factory");
            return false;
        }

        m_hwnd = static_cast<HWND>(windowHandle);
        m_width = width;
        m_height = height;
        m_vsync = vsync;

        // Assign borrowed pointers - ComPtr AddRef keeps them alive
        m_device.Attach(device);   device->AddRef();
        m_factory.Attach(factory); factory->AddRef();
        m_ownsDevice = false;

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        UWU_ENGINE_INFO("[DX12] InitShared {}x{} (shared device)", width, height);
        return InitSwapChainAndResources(m_hwnd, m_vsync);
    }

    bool DX12Renderer::InitSwapChainAndResources(HWND hwnd, bool vsync)
    {
        if (!CreateCommandObjects())     return false;
        if (!CreateSwapChain(hwnd))      return false;
        if (!CreateRTVHeap())            return false;
        if (!CreateDSVHeap())            return false;
        if (!CreateRenderTargetViews())  return false;
        if (!CreateDepthStencilBuffer()) return false;

        m_ready = true;
        UWU_ENGINE_INFO("[DX12] Renderer ready");
        return true;
    }


    void DX12Renderer::Shutdown()
    {
        if (!m_ready) return;
        FlushCommandQueue();

        if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }

        for (auto& rt : m_renderTargets) rt.Reset();
        m_depthStencilBuffer.Reset();
        m_rtvHeap.Reset();
        m_dsvHeap.Reset();
        m_swapChain.Reset();
        m_commandList.Reset();
        m_commandAllocator.Reset();
        m_commandQueue.Reset();
        m_fence.Reset();

        // Only release device/factory if we own them
        if (!m_ownsDevice)
        {
            m_device.Detach();   // already released by ComPtr when leaving scope
            m_factory.Detach();
        }

        m_ready = false;
        UWU_ENGINE_INFO("[DX12] Renderer shut down");
    }

    void DX12Renderer::OnResize(uint32_t width, uint32_t height)
    {
        if (!m_ready || width == 0 || height == 0) return;
        UWU_ENGINE_INFO("[DX12] Resize -> {}x{}", width, height);

        m_width = width;
        m_height = height;
        FlushCommandQueue();

        for (auto& rt : m_renderTargets) rt.Reset();
        m_depthStencilBuffer.Reset();

        HRESULT hr = m_swapChain->ResizeBuffers(
            kSwapChainBuffers, width, height,
            m_backBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);

        if (FAILED(hr))
        {
            UWU_ENGINE_ERROR("[DX12] ResizeBuffers failed hr={:#x}", (unsigned)hr);
            return;
        }

        m_backBufferIndex = 0;
        CreateRenderTargetViews();
        CreateDepthStencilBuffer();
    }

    void DX12Renderer::BeginFrame()
    {
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);

        D3D12_VIEWPORT vp{ 0, 0,
            static_cast<float>(m_width), static_cast<float>(m_height), 0.f, 1.f };
        D3D12_RECT scissor{ 0, 0,
            static_cast<LONG>(m_width), static_cast<LONG>(m_height) };

        m_commandList->RSSetViewports(1, &vp);
        m_commandList->RSSetScissorRects(1, &scissor);

        auto rtv = CurrentBackBufferRTV();
        auto dsv = DepthStencilView();
        m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        m_commandList->ClearRenderTargetView(rtv, m_clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsv,
            D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);
    }

    void DX12Renderer::EndFrame()
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);
        m_commandList->Close();

        ID3D12CommandList* lists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, lists);

        m_swapChain->Present(m_vsync ? 1 : 0, 0);

        WaitForPreviousFrame();
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    std::unique_ptr<IDrawable> DX12Renderer::CreateDrawable(const DrawableDesc& desc)
    {
        if (desc.mesh.primitive != PrimitiveType::Triangle)
        {
            UWU_ENGINE_WARN("[DX12] CreateDrawable: unsupported primitive type");
            return nullptr;
        }

        auto triangle = std::make_unique<DX12Triangle>();
        if (!triangle->Init(this, desc))
            return nullptr;
        return triangle;
    }

    // Private init

    bool DX12Renderer::CreateDebugLayer()
    {
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            UWU_ENGINE_INFO("[DX12] Debug layer enabled");
            return true;
        }
        return false;
    }

    bool DX12Renderer::CreateFactory()
    {
        UINT flags = 0;
#ifdef _DEBUG
        flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
        CHECK_HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
        UWU_ENGINE_INFO("[DX12] DXGI factory created");
        return true;
    }

    bool DX12Renderer::CreateDevice()
    {
        ComPtr<IDXGIAdapter1> adapter;
        bool found = false;

        for (UINT i = 0;
            m_factory->EnumAdapterByGpuPreference(i,
                DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
            ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
                D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
            {
                std::wstring ws(desc.Description);
                UWU_ENGINE_INFO("[DX12] Adapter: {}", NarrowAscii(ws));
                found = true;
                break;
            }
        }

        if (!found)
        {
            UWU_ENGINE_WARN("[DX12] No hardware adapter, using WARP");
            ComPtr<IDXGIAdapter> warp;
            CHECK_HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
            CHECK_HR(D3D12CreateDevice(warp.Get(),
                D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
        }

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        return true;
    }

    bool DX12Renderer::CreateCommandObjects()
    {
        D3D12_COMMAND_QUEUE_DESC qDesc{};
        qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        CHECK_HR(m_device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&m_commandQueue)));

        CHECK_HR(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

        CHECK_HR(m_device->CreateCommandList(0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr,
            IID_PPV_ARGS(&m_commandList)));
        m_commandList->Close();

        CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!m_fenceEvent) { UWU_ENGINE_ERROR("[DX12] CreateEvent failed"); return false; }

        UWU_ENGINE_INFO("[DX12] Command objects created");
        return true;
    }

    bool DX12Renderer::CreateSwapChain(HWND hwnd)
    {
        m_swapChain.Reset();

        DXGI_SWAP_CHAIN_DESC1 sd{};
        sd.BufferCount = kSwapChainBuffers;
        sd.Width = m_width;
        sd.Height = m_height;
        sd.Format = m_backBufferFormat;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.SampleDesc = { 1, 0 };

        ComPtr<IDXGISwapChain1> sc1;
        CHECK_HR(m_factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(), hwnd, &sd, nullptr, nullptr, &sc1));
        CHECK_HR(sc1.As(&m_swapChain));

        m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
        m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        UWU_ENGINE_INFO("[DX12] Swap chain created ({} buffers)", kSwapChainBuffers);
        return true;
    }

    bool DX12Renderer::CreateRTVHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC d{};
        d.NumDescriptors = kSwapChainBuffers;
        d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        CHECK_HR(m_device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&m_rtvHeap)));
        return true;
    }

    bool DX12Renderer::CreateDSVHeap()
    {
        D3D12_DESCRIPTOR_HEAP_DESC d{};
        d.NumDescriptors = 1;
        d.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        d.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        CHECK_HR(m_device->CreateDescriptorHeap(&d, IID_PPV_ARGS(&m_dsvHeap)));
        return true;
    }

    bool DX12Renderer::CreateRenderTargetViews()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE h(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        for (uint32_t i = 0; i < kSwapChainBuffers; ++i)
        {
            CHECK_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, h);
            h.Offset(1, m_rtvDescriptorSize);
        }
        return true;
    }

    bool DX12Renderer::CreateDepthStencilBuffer()
    {
        D3D12_RESOURCE_DESC dd{};
        dd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dd.Width = m_width;
        dd.Height = m_height;
        dd.DepthOrArraySize = 1;
        dd.MipLevels = 1;
        dd.Format = m_depthStencilFormat;
        dd.SampleDesc = { 1, 0 };
        dd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE cv{};
        cv.Format = m_depthStencilFormat;
        cv.DepthStencil.Depth = 1.f;
        cv.DepthStencil.Stencil = 0;

        auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        CHECK_HR(m_device->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &dd,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &cv, IID_PPV_ARGS(&m_depthStencilBuffer)));

        m_device->CreateDepthStencilView(
            m_depthStencilBuffer.Get(), nullptr, DepthStencilView());

        UWU_ENGINE_INFO("[DX12] Depth-stencil buffer created");
        return true;
    }

    // Private sync

    void DX12Renderer::FlushCommandQueue()
    {
        ++m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void DX12Renderer::WaitForPreviousFrame()
    {
        FlushCommandQueue();
    }

    // Private descriptor handles

    D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::CurrentBackBufferRTV() const
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(m_backBufferIndex),
            m_rtvDescriptorSize);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DX12Renderer::DepthStencilView() const
    {
        return m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    }

    ID3D12Resource* DX12Renderer::CurrentBackBuffer() const
    {
        return m_renderTargets[m_backBufferIndex].Get();
    }
}
