#pragma once
#include "Engine/Renderer/IDrawable.h"
#include "Engine/Renderer/DirectX12/DX12MeshGeometry.h"
#include "Engine/Renderer/DirectX12/DX12Shader.h"
#include "Engine/Renderer/DirectX12/DX12PipelineState.h"

namespace UwU_Engine
{
    class DX12Renderer;

    class UWU_API DX12Triangle final : public IDrawable
    {
    public:
        // Init stores the renderer pointer for use in Draw().
        // The renderer must outlive this drawable.
        bool Init(DX12Renderer* renderer, const DrawableDesc& desc);
        bool IsReady() const override { return m_ready; }

        // Uploads the current transform to the GPU CB and records draw commands.
        // Gets the command list from the stored renderer - no raw DX12 in callers.
        void Draw() override;

        void Shutdown() override;

        void SetTransform(const ObjectTransform& t) override { m_transform = t; }
        const ObjectTransform& GetTransform() const override { return m_transform; }

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool BuildShadersAndInputLayout(const DrawableDesc& desc);
        bool BuildGeometry(ID3D12Device* device, const DrawableDesc& desc);
        bool BuildTexture(ID3D12Device* device, const DrawableDesc& desc);
        bool BuildConstantBuffer(ID3D12Device* device);
        bool BuildPSO(ID3D12Device* device);

    private:
        DX12Renderer* m_renderer = nullptr; // borrowed, not owned

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_textureResource;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_textureUploadBuffer;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

        Microsoft::WRL::ComPtr<ID3D12Resource>      m_cbResource;
        void* m_cbMapped = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS                   m_cbGPUAddress = 0;

        DX12MeshGeometry                      m_geometry;
        DX12Shader                            m_vs;
        DX12Shader                            m_ps;
        DX12PipelineState                     m_pso;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
        D3D12_PRIMITIVE_TOPOLOGY              m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE         m_primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        ObjectTransform m_transform;
        bool            m_ready = false;
    };
}
