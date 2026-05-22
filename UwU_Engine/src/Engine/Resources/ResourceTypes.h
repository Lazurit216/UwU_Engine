#pragma once

#include "Engine/Renderer/IDrawable.h"

namespace UwU_Engine
{
    struct MeshMaterialData
    {
        Color4 diffuseColor;
        std::string diffuseTexturePath;
        bool hasDiffuseColor = false;
        bool hasDiffuseTexture = false;
    };

    struct MeshAsset
    {
        MeshData mesh;
        MeshMaterialData material;
    };

    struct TextureData
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<uint8_t> pixels;
    };

    struct TextureAsset
    {
        TextureData texture;
    };

    struct ShaderProgramData
    {
        std::string sourcePath;
        std::string vertexEntry = "VS";
        std::string pixelEntry = "PS";
        std::string vertexProfile = "vs_5_0";
        std::string pixelProfile = "ps_5_0";
        std::string sourceCode;
    };

    struct ShaderAsset
    {
        ShaderProgramData shader;
    };

    struct MaterialAsset
    {
        MaterialDesc material;
        bool hasBaseColor = false;
        bool hasShaderPath = false;
        bool hasTexturePath = false;
    };
}
