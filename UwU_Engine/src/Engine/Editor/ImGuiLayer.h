#pragma once

#include "Engine/Core.h"

#include <d3d12.h>

struct ImGui_ImplDX12_InitInfo;

namespace UwU_Engine
{
    class DX12Renderer;

    class UWU_API ImGuiLayer
    {
    public:
        ImGuiLayer();
        ~ImGuiLayer();

        bool Init(void* windowHandle, DX12Renderer& renderer);
        void Shutdown();

        void BeginFrame();
        void EndFrame(DX12Renderer& renderer);

        bool IsReady() const { return m_ready; }

    private:
        void ApplyEditorStyle();
        static void AllocateSrvDescriptor(
            ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
        static void FreeSrvDescriptor(
            ImGui_ImplDX12_InitInfo* info,
            D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
            D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    private:
        struct Data;

        std::unique_ptr<Data> m_data;
        bool m_ready = false;
    };
}
