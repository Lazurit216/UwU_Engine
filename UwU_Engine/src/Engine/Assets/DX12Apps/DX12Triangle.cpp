#include "uwupch.h"
#include "DX12Triangle.h"
#include "Engine/Renderer/DirectX12/DX12Renderer.h"
#include "Engine/Renderer/DirectX12/d3dx12.h"

using namespace Microsoft::WRL;
using namespace DirectX;

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

    bool DX12Triangle::Init(DX12Renderer* renderer, const TriangleDesc& desc)
    {
        UWU_ENGINE_INFO("[DX12Triangle] Init called - compiling shaders from: {}",
            NarrowAscii(desc.shaderPath));

        if (!renderer || !renderer->IsReady())
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Renderer is null or not ready");
            return false;
        }
        m_renderer = renderer;
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

    void DX12Triangle::Draw()
    {
        if (!m_ready || !m_renderer)
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Renderer doesn't exist or app don't ready");
            return;
        }
        static int calls = 0;
        if (calls++ < 3)
            UWU_ENGINE_INFO("[DX12Triangle] Draw() call #{} - renderer={} cmdList={}",
                calls,
                m_renderer ? "OK" : "NULL",
                m_renderer->GetCommandList() ? "OK" : "NULL");
        
        const float width = static_cast<float>(m_renderer->GetWidth());
        const float height = static_cast<float>(m_renderer->GetHeight());
        const float aspect = (height > 0.0f) ? width / height : 1.0f;

        XMMATRIX world =
            XMMatrixScaling(m_transform.scale, m_transform.scale, 1.f)
            * XMMatrixScaling(aspect, 1.f, 1.f)
            * XMMatrixRotationZ(m_transform.rotation)
            * XMMatrixScaling(1.f / aspect, 1.f, 1.f)
            * XMMatrixTranslation(m_transform.x, m_transform.y, 0.f);

        memcpy(m_cbMapped, &world, sizeof(XMMATRIX));

        // Fetch the live command list from the renderer
        ID3D12GraphicsCommandList* cmd = m_renderer->GetCommandList();

        cmd->SetPipelineState(m_pso.PSO.Get());
        cmd->SetGraphicsRootSignature(m_rootSignature.Get());
        cmd->SetGraphicsRootConstantBufferView(0, m_cbGPUAddress);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &m_vbView);
        cmd->IASetIndexBuffer(&m_ibView);
        cmd->DrawIndexedInstanced(3, 1, 0, 0, 0);
    }

    void DX12Triangle::Shutdown()
    {
        if (m_cbMapped) { m_cbResource->Unmap(0, nullptr); m_cbMapped = nullptr; }
        m_cbResource.Reset();
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_rootSignature.Reset();
        m_renderer = nullptr;
        m_ready = false;
        UWU_ENGINE_INFO("[DX12Triangle] Shutdown");
    }

    bool DX12Triangle::CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_ROOT_PARAMETER params[1];
        params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_ROOT_SIGNATURE_DESC desc(1, params, 0, nullptr,
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
        hr = device->CreateRootSignature(0, serialized->GetBufferPointer(),
            serialized->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
        if (FAILED(hr)) { UWU_ENGINE_ERROR("[DX12Triangle] CreateRootSignature failed"); return false; }
        return true;
    }

    bool DX12Triangle::BuildShadersAndInputLayout(const TriangleDesc& desc)
    {
        if (!m_vs.CompileFromFile(desc.shaderPath, "VS", "vs_5_0")) return false;
        if (!m_ps.CompileFromFile(desc.shaderPath, "PS", "ps_5_0")) return false;

        m_inputLayout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        return true;
    }

    bool DX12Triangle::BuildGeometry(ID3D12Device* device, const TriangleDesc& desc)
    {
        // Convert renderer-agnostic TriangleVertex to DX12Vertex (XMFLOAT3)
        std::array<DX12Vertex, 3> verts;
        for (int i = 0; i < 3; ++i)
        {
            verts[i].Position = { desc.vertices[i].position[0],
                                  desc.vertices[i].position[1],
                                  desc.vertices[i].position[2] };
            verts[i].Color = { desc.vertices[i].color[0],
                                  desc.vertices[i].color[1],
                                  desc.vertices[i].color[2] };
        }

        auto MakeBuf = [&](const void* data, UINT bytes, ComPtr<ID3D12Resource>& buf) -> bool
            {
                auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                auto bd = CD3DX12_RESOURCE_DESC::Buffer(bytes);
                if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE,
                    &bd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buf))))
                    return false;
                void* mapped; buf->Map(0, nullptr, &mapped);
                memcpy(mapped, data, bytes); buf->Unmap(0, nullptr);
                return true;
            };

        const UINT vbSize = sizeof(DX12Vertex) * 3;
        if (!MakeBuf(verts.data(), vbSize, m_vertexBuffer)) return false;
        m_vbView = { m_vertexBuffer->GetGPUVirtualAddress(), vbSize, sizeof(DX12Vertex) };

        const std::array<uint32_t, 3> idx = { 0, 1, 2 };
        const UINT ibSize = sizeof(uint32_t) * 3;
        if (!MakeBuf(idx.data(), ibSize, m_indexBuffer)) return false;
        m_ibView = { m_indexBuffer->GetGPUVirtualAddress(), ibSize, DXGI_FORMAT_R32_UINT };

        return true;
    }

    bool DX12Triangle::BuildConstantBuffer(ID3D12Device* device)
    {
        const UINT cbSize = (sizeof(XMMATRIX) + 255) & ~255;
        auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto bd = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
        if (FAILED(device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cbResource))))
            return false;

        m_cbResource->Map(0, nullptr, &m_cbMapped);
        m_cbGPUAddress = m_cbResource->GetGPUVirtualAddress();

        XMMATRIX identity = XMMatrixIdentity();
        memcpy(m_cbMapped, &identity, sizeof(XMMATRIX));
        return true;
    }

    bool DX12Triangle::BuildPSO(ID3D12Device* device)
    {
        return m_pso.Build(device, m_rootSignature.Get(), m_vs, m_ps, m_inputLayout);
    }
}
