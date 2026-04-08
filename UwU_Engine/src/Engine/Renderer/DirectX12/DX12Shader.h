#pragma once
namespace UwU_Engine
{
    class DX12Shader
    {
    public:

        bool CompileFromFile(const std::wstring& filename,
            const std::string& entryPoint,
            const std::string& target);

        bool CompileFromString(const std::string& shaderCode,
            const std::string& entryPoint,
            const std::string& target);
    public:
        Microsoft::WRL::ComPtr<ID3DBlob> ByteCode;
    };
}