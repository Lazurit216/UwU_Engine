#include "uwupch.h"
#include "DX12MeshGeometry.h"
#include "d3dx12.h"


namespace UwU_Engine
{
    DX12MeshGeometry::MeshData DX12MeshGeometry::CreateColoredTriangle()
    {
        MeshData mesh;

        mesh.Vertices = {
        Vertex({ 0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.5f, 0.0f }),
        Vertex({ 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }),
        Vertex({-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f })
        };

        mesh.Indices32 = { 0, 1, 2 };

        return mesh;
    }

    DX12MeshGeometry::MeshData DX12MeshGeometry::CreateTriangle(float size)
    {
        MeshData mesh;

        float h = size * 0.5f;

        mesh.Vertices = {
        Vertex({ 0.0f,  h, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.5f, 0.0f }),
        Vertex({  h,   -h, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }),
        Vertex({ -h,   -h, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f })
        };

        mesh.Indices32 = { 0, 1, 2 };

        return mesh;
    }

    bool DX12MeshGeometry::Create(ID3D12Device* device, const MeshData& meshData)
    {
        if (!device || meshData.Vertices.empty())
            return false;

        VertexCount = static_cast<UINT>(meshData.Vertices.size());
        IndexCount = static_cast<UINT>(meshData.Indices32.size());

        const UINT vbByteSize = VertexCount * sizeof(Vertex);
        const UINT ibByteSize = IndexCount * sizeof(std::uint32_t);

        // --- Vertex Buffer ---
        {
            auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vbByteSize);

            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&VertexBufferGPU));

            if (FAILED(hr))
            {
                UWU_ENGINE_ERROR("[DX12MeshGeometry] Failed to create Vertex Buffer");
                return false;
            }

            void* mappedData = nullptr;
            VertexBufferGPU->Map(0, nullptr, &mappedData);
            memcpy(mappedData, meshData.Vertices.data(), vbByteSize);
            VertexBufferGPU->Unmap(0, nullptr);

            VertexBufferView.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
            VertexBufferView.StrideInBytes = sizeof(Vertex);
            VertexBufferView.SizeInBytes = vbByteSize;
        }

        // --- Index Buffer ---
        if (IndexCount > 0)
        {
            auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(ibByteSize);

            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&IndexBufferGPU));

            if (FAILED(hr))
            {
                UWU_ENGINE_ERROR("[DX12MeshGeometry] Failed to create Index Buffer");
                return false;
            }

            void* mappedData = nullptr;
            IndexBufferGPU->Map(0, nullptr, &mappedData);
            memcpy(mappedData, meshData.Indices32.data(), ibByteSize);
            IndexBufferGPU->Unmap(0, nullptr);

            IndexBufferView.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
            IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
            IndexBufferView.SizeInBytes = ibByteSize;
        }

        UWU_ENGINE_INFO("[DX12MeshGeometry] Created geometry: {} vertices, {} indices", VertexCount, IndexCount);
        return true;
    }
}