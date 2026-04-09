#pragma once
#include "Engine/Renderer/DirectX12/DX12Shader.h"
#include "Engine/Renderer/DirectX12/DX12PipelineState.h"

namespace UwU_Engine
{
    class DX12Renderer;

    // Simple vertex used only by DX12Triangle.
    // Separate from DX12MeshGeometry::Vertex — no Normal/TexC needed here.
    struct TriangleVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    // All parameters the caller can customize before Init().
    struct TriangleDesc
    {
        // Vertex positions and colors — change these to get different triangles
        std::array<TriangleVertex, 3> Vertices = { {
            {{ 0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }}, // top   — red
            {{ 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }}, // right — green
            {{-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }}, // left  — blue
        } };

        std::wstring ShaderPath = L"Shaders/Triangle.hlsl";
    };

    // Luna-style test drawable.
    // Owns geometry (simple VB/IB), shaders, PSO and root signature.
    // NOT a renderer — takes DX12Renderer* for device access only.
    class DX12Triangle
    {
    public:
        bool Init(DX12Renderer* renderer, const TriangleDesc& desc = {});
        void Draw(ID3D12GraphicsCommandList* cmdList) const;
        void Shutdown();
        bool IsReady() const { return m_ready; }

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool BuildShadersAndInputLayout(const TriangleDesc& desc);
        bool BuildGeometry(ID3D12Device* device, const TriangleDesc& desc);
        bool BuildPSO(ID3D12Device* device);

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW                    m_vbView{};
        D3D12_INDEX_BUFFER_VIEW                     m_ibView{};

        DX12Shader                           m_vs;
        DX12Shader                           m_ps;
        DX12PipelineState                    m_pso;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

        bool m_ready = false;
    };
}