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

    bool DX12Triangle::Init(DX12Renderer* renderer, const DrawableDesc& desc)
    {
        UWU_ENGINE_INFO("[DX12Triangle] Init called - compiling shaders from: {}",
            NarrowAscii(desc.material.shaderPath));

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
        if (!BuildTexture(device, desc)) { UWU_ENGINE_ERROR("[DX12Triangle] Texture failed");           return false; }
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
            * XMMatrixRotationY(m_transform.rotationY)
            * XMMatrixRotationZ(m_transform.rotation)
            * XMMatrixScaling(1.f / aspect, 1.f, 1.f)
            * XMMatrixTranslation(m_transform.x, m_transform.y, 0.f);

        memcpy(m_cbMapped, &world, sizeof(XMMATRIX));

        // Fetch the live command list from the renderer
        ID3D12GraphicsCommandList* cmd = m_renderer->GetCommandList();

        cmd->SetPipelineState(m_pso.PSO.Get());
        cmd->SetGraphicsRootSignature(m_rootSignature.Get());
        ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvHeap.Get() };
        cmd->SetDescriptorHeaps(1, descriptorHeaps);
        cmd->SetGraphicsRootConstantBufferView(0, m_cbGPUAddress);
        cmd->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &m_vbView);
        cmd->IASetIndexBuffer(&m_ibView);
        cmd->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
    }

    void DX12Triangle::Shutdown()
    {
        if (m_cbMapped) { m_cbResource->Unmap(0, nullptr); m_cbMapped = nullptr; }
        m_cbResource.Reset();
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_textureResource.Reset();
        m_textureUploadBuffer.Reset();
        m_srvHeap.Reset();
        m_rootSignature.Reset();
        m_renderer = nullptr;
        m_ready = false;
        UWU_ENGINE_INFO("[DX12Triangle] Shutdown");
    }

    bool DX12Triangle::CreateRootSignature(ID3D12Device* device)
    {
        CD3DX12_DESCRIPTOR_RANGE textureTable;
        textureTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        CD3DX12_ROOT_PARAMETER params[2];
        params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        params[1].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC sampler(
            0,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP);

        CD3DX12_ROOT_SIGNATURE_DESC desc(2, params, 1, &sampler,
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

    bool DX12Triangle::BuildShadersAndInputLayout(const DrawableDesc& desc)
    {
        if (!m_vs.CompileFromFile(desc.material.shaderPath, "VS", "vs_5_0")) return false;
        if (!m_ps.CompileFromFile(desc.material.shaderPath, "PS", "ps_5_0")) return false;

        m_inputLayout =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
        return true;
    }

    bool DX12Triangle::BuildGeometry(ID3D12Device* device, const DrawableDesc& desc)
    {
        if (desc.mesh.vertices.size() < 3 || desc.mesh.indices.size() < 3)
        {
            UWU_ENGINE_ERROR("[DX12Triangle] Drawable requires at least 3 vertices and 3 indices");
            return false;
        }

        std::vector<DX12Vertex> verts;
        verts.resize(desc.mesh.vertices.size());
        for (size_t i = 0; i < verts.size(); ++i)
        {
            verts[i].Position = { desc.mesh.vertices[i].position[0],
                                  desc.mesh.vertices[i].position[1],
                                  desc.mesh.vertices[i].position[2] };
            verts[i].Color = { desc.mesh.vertices[i].color[0],
                               desc.mesh.vertices[i].color[1],
                               desc.mesh.vertices[i].color[2] };
            verts[i].TexCoord = { desc.mesh.vertices[i].uv[0],
                                  desc.mesh.vertices[i].uv[1] };
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

        const UINT vbSize = static_cast<UINT>(sizeof(DX12Vertex) * verts.size());
        if (!MakeBuf(verts.data(), vbSize, m_vertexBuffer)) return false;
        m_vbView = { m_vertexBuffer->GetGPUVirtualAddress(), vbSize, sizeof(DX12Vertex) };

        const UINT ibSize = static_cast<UINT>(sizeof(uint32_t) * desc.mesh.indices.size());
        if (!MakeBuf(desc.mesh.indices.data(), ibSize, m_indexBuffer)) return false;
        m_ibView = { m_indexBuffer->GetGPUVirtualAddress(), ibSize, DXGI_FORMAT_R32_UINT };
        m_indexCount = static_cast<UINT>(desc.mesh.indices.size());

        return true;
    }

    bool DX12Triangle::BuildTexture(ID3D12Device* device, const DrawableDesc& desc)
    {
        std::vector<uint8_t> fallbackWhite = { 255, 255, 255, 255 };
        const uint8_t* pixels = fallbackWhite.data();
        int width = 1;
        int height = 1;
        int channels = 4;

        if (!desc.material.texturePixels.empty()
            && desc.material.textureWidth > 0
            && desc.material.textureHeight > 0)
        {
            pixels = desc.material.texturePixels.data();
            width = desc.material.textureWidth;
            height = desc.material.textureHeight;
            channels = desc.material.textureChannels;
        }

        if (channels != 4)
        {
            UWU_ENGINE_WARN("[DX12Triangle] Texture is expected to be RGBA8, got {} channels", channels);
            pixels = fallbackWhite.data();
            width = 1;
            height = 1;
        }

        D3D12_RESOURCE_DESC textureDesc{};
        textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureDesc.Alignment = 0;
        textureDesc.Width = static_cast<UINT64>(width);
        textureDesc.Height = static_cast<UINT>(height);
        textureDesc.DepthOrArraySize = 1;
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.SampleDesc.Quality = 0;
        textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        HRESULT hr = device->CreateCommittedResource(
            &defaultHeap,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_textureResource));
        if (FAILED(hr))
            return false;

        const UINT64 uploadSize = GetRequiredIntermediateSize(m_textureResource.Get(), 0, 1);
        auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
        hr = device->CreateCommittedResource(
            &uploadHeap,
            D3D12_HEAP_FLAG_NONE,
            &uploadBufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_textureUploadBuffer));
        if (FAILED(hr))
            return false;

        D3D12_SUBRESOURCE_DATA textureData{};
        textureData.pData = pixels;
        textureData.RowPitch = static_cast<LONG_PTR>(width) * 4;
        textureData.SlicePitch = textureData.RowPitch * static_cast<LONG_PTR>(height);

        ID3D12GraphicsCommandList* commandList = m_renderer ? m_renderer->GetCommandList() : nullptr;
        if (!commandList)
            return false;

        UpdateSubresources(commandList, m_textureResource.Get(), m_textureUploadBuffer.Get(), 0, 0, 1, &textureData);
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textureResource.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.NumDescriptors = 1;
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_srvHeap));
        if (FAILED(hr))
            return false;

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(
            m_textureResource.Get(),
            &srvDesc,
            m_srvHeap->GetCPUDescriptorHandleForHeapStart());

        UWU_ENGINE_INFO("[DX12Triangle] Texture uploaded ({}x{})", width, height);
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
