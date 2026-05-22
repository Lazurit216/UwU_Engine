#include "uwupch.h"
#include "Engine/Editor/ImGuiLayer.h"

#include "Engine/Renderer/DirectX12/DX12Renderer.h"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

namespace UwU_Engine
{
    namespace
    {
        constexpr uint32_t kImGuiSrvDescriptorCount = 64;
    }

    struct ImGuiLayer::Data
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
        UINT descriptorSize = 0;
        uint32_t nextDescriptorIndex = 0;
        std::vector<uint32_t> freeDescriptorIndices;
        bool contextCreated = false;
        bool win32Initialized = false;
        bool dx12Initialized = false;
    };

    void ImGuiLayer::AllocateSrvDescriptor(
        ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
    {
        auto* data = static_cast<ImGuiLayer::Data*>(info->UserData);
        uint32_t descriptorIndex = 0;

        if (!data->freeDescriptorIndices.empty())
        {
            descriptorIndex = data->freeDescriptorIndices.back();
            data->freeDescriptorIndices.pop_back();
        }
        else
        {
            descriptorIndex = data->nextDescriptorIndex++;
        }

        if (descriptorIndex >= kImGuiSrvDescriptorCount)
        {
            UWU_ENGINE_ERROR("[ImGui] SRV descriptor heap is full");
            *outCpuHandle = {};
            *outGpuHandle = {};
            return;
        }

        *outCpuHandle = data->srvHeap->GetCPUDescriptorHandleForHeapStart();
        outCpuHandle->ptr += static_cast<SIZE_T>(descriptorIndex) * data->descriptorSize;

        *outGpuHandle = data->srvHeap->GetGPUDescriptorHandleForHeapStart();
        outGpuHandle->ptr += static_cast<UINT64>(descriptorIndex) * data->descriptorSize;
    }

    void ImGuiLayer::FreeSrvDescriptor(
        ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE)
    {
        auto* data = static_cast<ImGuiLayer::Data*>(info->UserData);
        const auto heapStart = data->srvHeap->GetCPUDescriptorHandleForHeapStart().ptr;
        const auto descriptorIndex = static_cast<uint32_t>((cpuHandle.ptr - heapStart) / data->descriptorSize);
        if (descriptorIndex < kImGuiSrvDescriptorCount)
            data->freeDescriptorIndices.push_back(descriptorIndex);
    }

    ImGuiLayer::ImGuiLayer() = default;

    ImGuiLayer::~ImGuiLayer()
    {
        Shutdown();
    }

    bool ImGuiLayer::Init(void* windowHandle, DX12Renderer& renderer)
    {
        if (m_ready)
            return true;

        if (!windowHandle || !renderer.GetDevice() || !renderer.GetCommandList())
        {
            UWU_ENGINE_ERROR("[ImGui] Init failed: invalid window or renderer");
            return false;
        }

        m_data = std::make_unique<Data>();

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = kImGuiSrvDescriptorCount;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        HRESULT hr = renderer.GetDevice()->CreateDescriptorHeap(
            &heapDesc,
            IID_PPV_ARGS(&m_data->srvHeap));
        if (FAILED(hr))
        {
            UWU_ENGINE_ERROR("[ImGui] Failed to create SRV descriptor heap");
            Shutdown();
            return false;
        }
        m_data->descriptorSize = renderer.GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        m_data->contextCreated = true;

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ApplyEditorStyle();

        if (!ImGui_ImplWin32_Init(windowHandle))
        {
            UWU_ENGINE_ERROR("[ImGui] Win32 backend init failed");
            Shutdown();
            return false;
        }
        m_data->win32Initialized = true;

        ImGui_ImplDX12_InitInfo initInfo{};
        initInfo.Device = renderer.GetDevice();
        initInfo.CommandQueue = renderer.GetCommandQueue();
        initInfo.NumFramesInFlight = static_cast<int>(renderer.GetSwapChainBufferCount());
        initInfo.RTVFormat = renderer.GetBackBufferFormat();
        initInfo.DSVFormat = renderer.GetDepthStencilFormat();
        initInfo.SrvDescriptorHeap = m_data->srvHeap.Get();
        initInfo.SrvDescriptorAllocFn = AllocateSrvDescriptor;
        initInfo.SrvDescriptorFreeFn = FreeSrvDescriptor;
        initInfo.UserData = m_data.get();

        const bool dx12Ok = ImGui_ImplDX12_Init(&initInfo);

        if (!dx12Ok)
        {
            UWU_ENGINE_ERROR("[ImGui] DX12 backend init failed");
            Shutdown();
            return false;
        }
        m_data->dx12Initialized = true;

        m_ready = true;
        UWU_ENGINE_INFO("[ImGui] Layer initialized");
        return true;
    }

    void ImGuiLayer::Shutdown()
    {
        if (!m_data)
            return;

        if (m_data->dx12Initialized)
            ImGui_ImplDX12_Shutdown();
        if (m_data->win32Initialized)
            ImGui_ImplWin32_Shutdown();
        if (m_data->contextCreated)
            ImGui::DestroyContext();

        m_data.reset();
        m_ready = false;
    }

    void ImGuiLayer::BeginFrame()
    {
        if (!m_ready)
            return;

        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame(DX12Renderer& renderer)
    {
        if (!m_ready)
            return;

        ImGui::Render();

        ID3D12DescriptorHeap* heaps[] = { m_data->srvHeap.Get() };
        renderer.GetCommandList()->SetDescriptorHeaps(1, heaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), renderer.GetCommandList());
    }

    void ImGuiLayer::ApplyEditorStyle()
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 3.0f;
        style.ChildRounding = 3.0f;
        style.FrameRounding = 2.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 2.0f;
        style.GrabRounding = 2.0f;
        style.TabRounding = 2.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.ItemSpacing = ImVec2(8.0f, 5.0f);
        style.WindowPadding = ImVec2(8.0f, 8.0f);

        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.09f, 0.09f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.31f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.34f, 0.38f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.34f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.36f, 0.62f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.20f, 0.23f, 1.00f);
    }
}
