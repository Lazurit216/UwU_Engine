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

        if (!CreateRootSignature(device)) { UWU_ENGINE_ERROR("[DX12Triangle] Root signature failed");  return false; }
        if (!BuildShadersAndInputLayout(desc)) { UWU_ENGINE_ERROR("[DX12Triangle] Shaders/layout failed");  return false; }
        if (!BuildGeometry(device, desc)) { UWU_ENGINE_ERROR("[DX12Triangle] Geometry failed");         return false; }
        if (!BuildConstantBuffer(device)) { UWU_ENGINE_ERROR("[DX12Triangle] Constant buffer failed");  return false; }
        if (!BuildPSO(device)) { UWU_ENGINE_ERROR("[DX12Triangle] PSO failed");               return false; }

        m_ready = true;
        UWU_ENGINE_INFO("[DX12Triangle] Initialized");
        return true;
    }

    void DX12Triangle::Draw(ID3D12GraphicsCommandList* cmdList) const
    {
        if (!m_ready || !cmdList) return;

        XMMATRIX world = m_transform.ToMatrix();
        memcpy(m_cbMapped, &world, sizeof(XMMATRIX));

        cmdList->SetPipelineState(m_pso.PSO.Get());
        cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
        cmdList->SetGraphicsRootConstantBufferView(0, m_cbGPUAddress);
        cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmdList->IASetVertexBuffers(0, 1, &m_vbView);
        cmdList->IASetIndexBuffer(&m_ibView);
        cmdList->DrawIndexedInstanced(3, 1, 0, 0, 0);
    }

    void DX12Triangle::Shutdown()
    {
        if (m_cbMapped) { m_cbResource->Unmap(0, nullptr); m_cbMapped = nullptr; }
        m_cbResource.Reset();
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_rootSignature.Reset();
        m_ready = false;
        UWU_ENGINE_INFO("[DX12Triangle] Shutdown");
    }

    bool DX12Triangle::CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_ROOT_PARAMETER params[1];
        params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_ROOT_SIGNATURE_DESC desc(
            1, params, 0, nullptr,
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> serialized, errors;
        HRESULT hr = D3D12SerializeRootSignature(
            &desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);

        if (FAILED(hr))
        {
            if (errors) UWU_ENGINE_ERROR("[DX12Triangle] Root sig: {}",
                (char*)errors->GetBufferPointer());
            return false;
        }
        hr = device->CreateRootSignature(0,
            serialized->GetBufferPointer(), serialized->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr)) { UWU_ENGINE_ERROR("[DX12Triangle] CreateRootSignature failed"); return false; }
        return true;
    }

    bool DX12Triangle::BuildShadersAndInputLayout(const TriangleDesc& desc)
    {
        // Use desc.ShaderPath — was hardcoded before
        if (!m_vs.CompileFromFile(desc.ShaderPath, "VS", "vs_5_0")) return false;
        if (!m_ps.CompileFromFile(desc.ShaderPath, "PS", "ps_5_0")) return false;

        m_inputLayout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        return true;
    }

    bool DX12Triangle::BuildGeometry(ID3D12Device* device, const TriangleDesc& desc)
    {
        auto CreateBuffer = [device](const void* data, UINT bytes,
            ComPtr<ID3D12Resource>& buffer) -> bool
            {
                auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bytes);

                HRESULT hr = device->CreateCommittedResource(
                    &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                    IID_PPV_ARGS(&buffer));

                if (FAILED(hr)) return false;

                void* mapped = nullptr;
                buffer->Map(0, nullptr, &mapped);
                memcpy(mapped, data, bytes);
                buffer->Unmap(0, nullptr);

                return true;
            };

        // Vertex buffer
        const UINT vbSize = sizeof(TriangleVertex) * 3;
        if (!CreateBuffer(desc.Vertices.data(), vbSize, m_vertexBuffer)) return false;
        m_vbView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vbView.StrideInBytes = sizeof(TriangleVertex);
        m_vbView.SizeInBytes = vbSize;

        // Index buffer
        const std::array<uint32_t, 3> indices = { 0, 1, 2 };
        const UINT ibSize = sizeof(uint32_t) * 3;
        if (!CreateBuffer(indices.data(), ibSize, m_indexBuffer)) return false;
        m_ibView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_ibView.Format = DXGI_FORMAT_R32_UINT;
        m_ibView.SizeInBytes = ibSize;

        return true;
    }

    bool DX12Triangle::BuildConstantBuffer(ID3D12Device* device)
    {
        const UINT cbSize = (sizeof(XMMATRIX) + 255) & ~255;  // 256 bytes

        auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bd = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cbResource))))
            return false;

        m_cbResource->Map(0, nullptr, &m_cbMapped);          // stays mapped forever
        m_cbGPUAddress = m_cbResource->GetGPUVirtualAddress();

        XMMATRIX identity = XMMatrixIdentity();              // identity until first Draw
        memcpy(m_cbMapped, &identity, sizeof(XMMATRIX));
        return true;
    }

    bool DX12Triangle::BuildPSO(ID3D12Device* device)
    {
        return m_pso.Build(device, m_rootSignature.Get(),
            m_vs, m_ps, m_inputLayout);
    }
}