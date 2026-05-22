#include "uwupch.h"
#include "DX12PipelineState.h"
#include "d3dx12.h"

namespace UwU_Engine
{
    bool DX12PipelineState::Build(ID3D12Device* device,
        ID3D12RootSignature* rootSignature,
        const DX12Shader& vs,
        const DX12Shader& ps,
        const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = { vs.ByteCode->GetBufferPointer(), vs.ByteCode->GetBufferSize() };
        psoDesc.PS = { ps.ByteCode->GetBufferPointer(), ps.ByteCode->GetBufferSize() };
        psoDesc.PrimitiveTopologyType = topologyType;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

        HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&PSO));
        if (FAILED(hr))
        {
            UWU_ENGINE_ERROR("[DX12] Failed to create PSO");
            return false;
        }
        return true;
    }
}
