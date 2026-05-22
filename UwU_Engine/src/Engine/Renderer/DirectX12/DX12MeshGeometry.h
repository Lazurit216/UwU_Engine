#pragma once

#include "Engine/Renderer/IDrawable.h"

namespace UwU_Engine
{
    class DX12MeshGeometry
    {
    public:
        bool Create(ID3D12Device* device, const MeshData& meshData);
        void Shutdown();

        const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_vertexBufferView; }
        const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_indexBufferView; }
        UINT GetIndexCount() const { return m_indexCount; }

        static std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout();

    private:
        struct Vertex
        {
            DirectX::XMFLOAT3 Position;
            DirectX::XMFLOAT3 Color;
            DirectX::XMFLOAT2 TexCoord;
        };

        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};
        UINT m_indexCount = 0;
    };
}
