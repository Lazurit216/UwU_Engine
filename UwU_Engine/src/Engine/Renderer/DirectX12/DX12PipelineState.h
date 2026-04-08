#pragma once
#include "DX12Shader.h"

namespace UwU_Engine
{
    class DX12PipelineState
    {
    public:
        Microsoft::WRL::ComPtr<ID3D12PipelineState> PSO;

        bool Build(ID3D12Device* device,
            ID3D12RootSignature* rootSignature,
            const DX12Shader& vs,
            const DX12Shader& ps,
            const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);
    };
}