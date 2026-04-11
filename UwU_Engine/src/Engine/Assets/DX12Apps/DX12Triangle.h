#pragma once
#include "Engine/Renderer/DirectX12/DX12Shader.h"
#include "Engine/Renderer/DirectX12/DX12PipelineState.h"

namespace UwU_Engine
{
    class DX12Renderer;

    struct TriangleVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Color;
    };

    struct ObjectTransform
    {
        float x = 0.0f;   // horizontal offset  [-1.8 .. 1.8]
        float y = 0.0f;   // vertical offset    [-1.8 .. 1.8]
        float scale = 1.0f;   // uniform scale      [0.1 .. 5.0]
        float rotation = 0.0f;   // radians, CCW

        // Scale × RotateZ × Translate — passed to the vertex shader.
        DirectX::XMMATRIX ToMatrix() const
        {
            using namespace DirectX;
            return XMMatrixScaling(scale, scale, 1.f)
                * XMMatrixRotationZ(rotation)
                * XMMatrixTranslation(x, y, 0.f);
        }
    };

    struct TriangleDesc
    {
        // Vertex positions and colors — change these to get different triangles
        std::array<TriangleVertex, 3> Vertices = { {
            {{ 0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }}, // top   — red
            {{ 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }}, // right — green
            {{-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }}, // left  — blue
        } };

        std::wstring ShaderPath = L"Assets/Shaders/Color.hlsl";
    };

    // Luna-style test drawable.
    // Owns geometry (simple VB/IB), shaders, PSO and root signature.
    // NOT a renderer — takes DX12Renderer* for device access only.
    class UWU_API DX12Triangle
    {
    public:
        bool Init(DX12Renderer* renderer, const TriangleDesc& desc = {});
        void Draw(ID3D12GraphicsCommandList* cmdList) const;
        void Shutdown();
        bool IsReady() const { return m_ready; }

        void SetTransform(const ObjectTransform& t) { m_transform = t; }
        const ObjectTransform& GetTransform() const { return m_transform; }

    private:
        bool CreateRootSignature(ID3D12Device* device);
        bool BuildShadersAndInputLayout(const TriangleDesc& desc);
        bool BuildGeometry(ID3D12Device* device, const TriangleDesc& desc);
        bool BuildConstantBuffer(ID3D12Device* device);
        bool BuildPSO(ID3D12Device* device);

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vbView{};
        D3D12_INDEX_BUFFER_VIEW m_ibView{};

        DX12Shader m_vs;
        DX12Shader m_ps;
        DX12PipelineState m_pso;
        std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_cbResource;
        void* m_cbMapped = nullptr;
        D3D12_GPU_VIRTUAL_ADDRESS m_cbGPUAddress = 0;

        ObjectTransform m_transform;
        bool m_ready = false;
    };
}