#include "uwupch.h"
#include "DX12Shader.h"
namespace UwU_Engine
{
    bool DX12Shader::CompileFromFile(const std::wstring& filename,
        const std::string& entryPoint,
        const std::string& target)
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompileFromFile(filename.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), target.c_str(), flags, 0,
            &ByteCode, &errors);

        if (FAILED(hr))
        {
            if (errors)
                UWU_ENGINE_ERROR("Shader compilation failed: {}", (char*)errors->GetBufferPointer());
            return false;
        }
        return true;
    }

    bool DX12Shader::CompileFromString(const std::string& shaderCode,
        const std::string& entryPoint,
        const std::string& target)
    {
        UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG;
#endif

        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        HRESULT hr = D3DCompile(shaderCode.c_str(), shaderCode.size(), nullptr,
            nullptr, nullptr, entryPoint.c_str(), target.c_str(),
            flags, 0, &ByteCode, &errors);

        if (FAILED(hr))
        {
            if (errors)
                UWU_ENGINE_ERROR("Shader compilation failed: {}", (char*)errors->GetBufferPointer());
            return false;
        }
        return true;
    }
}