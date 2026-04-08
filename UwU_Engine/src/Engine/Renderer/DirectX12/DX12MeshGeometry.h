#pragma once

namespace UwU_Engine
{
    class DX12MeshGeometry
    {
    public:

        using uint16 = std::uint16_t;
        using uint32 = std::uint32_t;

        struct Vertex
        {
            Vertex() {}
            Vertex(
                const DirectX::XMFLOAT3& p,
                const DirectX::XMFLOAT3& n,
                const DirectX::XMFLOAT3& t,
                const DirectX::XMFLOAT2& uv) :
                Position(p),
                Normal(n),
                TangentU(t),
                TexC(uv) {}
            Vertex(
                float px, float py, float pz,
                float nx, float ny, float nz,
                float tx, float ty, float tz,
                float u, float v) :
                Position(px, py, pz),
                Normal(nx, ny, nz),
                TangentU(tx, ty, tz),
                TexC(u, v) {
            }

            DirectX::XMFLOAT3 Position;
            DirectX::XMFLOAT3 Normal;
            DirectX::XMFLOAT3 TangentU;
            DirectX::XMFLOAT2 TexC;
        };

        struct MeshData
        {
            std::vector<Vertex> Vertices;
            std::vector<uint32> Indices32;

            std::vector<uint16>& GetIndices16()
            {
                if (mIndices16.empty())
                {
                    mIndices16.resize(Indices32.size());
                    for (size_t i = 0; i < Indices32.size(); ++i)
                        mIndices16[i] = static_cast<uint16>(Indices32[i]);
                }

                return mIndices16;
            }

        private:
            std::vector<uint16> mIndices16;
        };

        bool Create(ID3D12Device* device, const MeshData& meshData);

        static MeshData CreateTriangle(float size = 1.0f);
        static MeshData CreateColoredTriangle();

    public:
        Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU;
        Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU;

        D3D12_VERTEX_BUFFER_VIEW VertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW  IndexBufferView{};

        UINT VertexCount = 0;
        UINT IndexCount = 0;
    };
}