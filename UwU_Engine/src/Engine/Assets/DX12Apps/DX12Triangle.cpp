#include "uwupch.h"
#include "DX12Triangle.h"
#include "Engine/Renderer/DirectX12/DX12Renderer.h"
#include "Engine/Renderer/DirectX12/d3dx12.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace UwU_Engine
{
    bool DX12Triangle::Init(DX12Renderer* renderer, const TriangleDesc& desc)
    {
        if (!renderer || !renderer->IsReady())
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Renderer is null or not ready");
            return false;
        }

        ID3D12Device* device = renderer->GetDevice();

        if (!CreateRootSignature(device))          return false;
        if (!BuildShadersAndInputLayout(desc))     return false;
        if (!BuildGeometry(device, desc))          return false;
        if (!BuildPSO(device))                     return false;

        m_ready = true;
        UWU_ENGINE_INFO("[DX12Triangle] Initialized");
        return true;
    }

    void DX12Triangle::Draw(ID3D12GraphicsCommandList* cmdList) const
    {
        if (!m_ready || !cmdList) return;

        cmdList->SetPipelineState(m_pso.PSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &m_vbView);
        cmdList->IASetIndexBuffer(&m_ibView);
        cmdList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    }

    void DX12Triangle::Shutdown()
    {
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_rootSignature.Reset();
        m_ready = false;
        UWU_ENGINE_INFO("[DX12Triangle] Shutdown");
    }

    bool DX12Triangle::CreateRootSignature(ID3D12Device* device)
    {
        // Empty root sig — no CBV/SRV/UAV needed for a colored triangle
        CD3DX12_ROOT_SIGNATURE_DESC desc(
            0, nullptr, 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> serialized, errors;
        HRESULT hr = D3D12SerializeRootSignature(
            &desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);

        if (FAILED(hr))
        {
            if (errors)
                UWU_ENGINE_ERROR("[DX12Triangle] Root sig error: {}",
                    (char*)errors->GetBufferPointer());
            return false;
        }

        hr = device->CreateRootSignature(
            0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature));

        if (FAILED(hr)) { UWU_ENGINE_ERROR("[DX12Triangle] CreateRootSignature failed"); return false; }
        return true;
    }

    bool DX12Triangle::BuildShadersAndInputLayout(const TriangleDesc& desc)
    {
        // Load both VS and PS from the same .hlsl file
        if (!m_vs.CompileFromFile(desc.ShaderPath, "VS", "vs_5_0")) return false;
        if (!m_ps.CompileFromFile(desc.ShaderPath, "PS", "ps_5_0")) return false;

        // Must match TriangleVertex layout exactly:
        // Position(12 bytes) + Color(12 bytes) = 24 bytes total
        m_inputLayout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
              D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        return true;
    }

    bool DX12Triangle::BuildGeometry(ID3D12Device* device, const TriangleDesc& desc)
    {
        // ── Vertex buffer ─────────────────────────────────────────────────────
        const UINT vbSize = sizeof(TriangleVertex) * 3;

        auto vbHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto vbDesc = CD3DX12_RESOURCE_DESC::Buffer(vbSize);

        if (FAILED(device->CreateCommittedResource(
            &vbHeap, D3D12_HEAP_FLAG_NONE, &vbDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_vertexBuffer))))
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Failed to create vertex buffer");
            return false;
        }

        void* mapped = nullptr;
        m_vertexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, desc.Vertices.data(), vbSize);
        m_vertexBuffer->Unmap(0, nullptr);

        m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vbView.StrideInBytes = sizeof(TriangleVertex);
        m_vbView.SizeInBytes = vbSize;

        // ── Index buffer ──────────────────────────────────────────────────────
        const std::array<uint32_t, 3> indices = { 0, 1, 2 };
        const UINT ibSize = sizeof(uint32_t) * 3;

        auto ibHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto ibDesc = CD3DX12_RESOURCE_DESC::Buffer(ibSize);

        if (FAILED(device->CreateCommittedResource(
            &ibHeap, D3D12_HEAP_FLAG_NONE, &ibDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_PPV_ARGS(&m_indexBuffer))))
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Failed to create index buffer");
            return false;
        }

        m_indexBuffer->Map(0, nullptr, &mapped);
        memcpy(mapped, indices.data(), ibSize);
        m_indexBuffer->Unmap(0, nullptr);

        m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_ibView.Format = DXGI_FORMAT_R32_UINT;
        m_ibView.SizeInBytes = ibSize;

        return true;
    }

    bool DX12Triangle::BuildPSO(ID3D12Device* device)
    {
        return m_pso.Build(device, m_rootSignature.Get(),
            m_vs, m_ps, m_inputLayout);
    }
}