#include "uwupch.h"
#include "DX12MeshGeometry.h"
#include "d3dx12.h"

namespace UwU_Engine
{
    namespace
    {
        bool CreateUploadBuffer(
            ID3D12Device* device,
            const void* data,
            UINT byteSize,
            Microsoft::WRL::ComPtr<ID3D12Resource>& buffer)
        {
            auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&buffer));

            if (FAILED(hr))
                return false;

            void* mappedData = nullptr;
            buffer->Map(0, nullptr, &mappedData);
            memcpy(mappedData, data, byteSize);
            buffer->Unmap(0, nullptr);
            return true;
        }
    }

    bool DX12MeshGeometry::Create(ID3D12Device* device, const MeshData& meshData)
    {
        if (!device)
            return false;

        if (meshData.vertices.size() < 3 || meshData.indices.size() < 3)
        {
            UWU_ENGINE_ERROR("[DX12MeshGeometry] Drawable requires at least 3 vertices and 3 indices");
            return false;
        }

        std::vector<Vertex> vertices;
        vertices.resize(meshData.vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vertices[i].Position = {
                meshData.vertices[i].position[0],
                meshData.vertices[i].position[1],
                meshData.vertices[i].position[2]
            };
            vertices[i].Color = {
                meshData.vertices[i].color[0],
                meshData.vertices[i].color[1],
                meshData.vertices[i].color[2]
            };
            vertices[i].TexCoord = {
                meshData.vertices[i].uv[0],
                meshData.vertices[i].uv[1]
            };
        }

        const UINT vertexBufferSize = static_cast<UINT>(sizeof(Vertex) * vertices.size());
        if (!CreateUploadBuffer(device, vertices.data(), vertexBufferSize, m_vertexBuffer))
        {
            UWU_ENGINE_ERROR("[DX12MeshGeometry] Failed to create vertex buffer");
            return false;
        }

        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        const UINT indexBufferSize = static_cast<UINT>(sizeof(uint32_t) * meshData.indices.size());
        if (!CreateUploadBuffer(device, meshData.indices.data(), indexBufferSize, m_indexBuffer))
        {
            UWU_ENGINE_ERROR("[DX12MeshGeometry] Failed to create index buffer");
            return false;
        }

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;
        m_indexCount = static_cast<UINT>(meshData.indices.size());

        UWU_ENGINE_INFO("[DX12MeshGeometry] Created geometry: {} vertices, {} indices",
            vertices.size(), m_indexCount);
        return true;
    }

    void DX12MeshGeometry::Shutdown()
    {
        m_vertexBuffer.Reset();
        m_indexBuffer.Reset();
        m_vertexBufferView = {};
        m_indexBufferView = {};
        m_indexCount = 0;
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> DX12MeshGeometry::GetInputLayout()
    {
        return {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };
    }
}
