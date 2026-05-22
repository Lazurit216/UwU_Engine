#pragma once
// Engine/Renderer/IDrawable.h
// Abstract drawable.  Sandbox and states work with IDrawable* only -
// they never need to see DX12Triangle or any command list.

#include "Engine/Core.h"

namespace UwU_Engine
{
    // World transform. ECS systems may also attach a camera view-projection matrix.
    struct UWU_API ObjectTransform
    {
        float x = 0.f;
        float y = 0.f;
        float z = 0.f;
        float scale = 1.f;   // uniform scale      [ 0.1 ..  5.0]
        float scaleX = 1.f;
        float scaleY = 1.f;
        float scaleZ = 1.f;
        float rotation = 0.f;   // radians, CCW
        float rotationX = 0.f;  // radians
        float rotationY = 0.f;  // radians
        float rotationZ = 0.f;  // radians
        bool useViewProjection = false;
        std::array<float, 16> viewProjection = {
            1.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 0.f,
            0.f, 0.f, 1.f, 0.f,
            0.f, 0.f, 0.f, 1.f
        };
    };

    struct Vertex
    {
        float position[3];
        float normal[3] = { 0.0f, 0.0f, 1.0f };
        float uv[2] = { 0.0f, 0.0f };
        float color[3];
    };

    enum class PrimitiveType
    {
        Line,
        Triangle,
        Quad,
        Cube,
        Box,
        Sphere,
        Plane,
        CustomMesh
    };

    struct Color4
    {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };

    struct MeshData
    {
        PrimitiveType primitive = PrimitiveType::Triangle;
        std::string sourcePath;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct MaterialDesc
    {
        Color4 baseColor;
        std::wstring shaderPath = L"Assets/Shaders/Color.hlsl";
        std::string shaderSourceCode;
        std::string vertexEntry = "VS";
        std::string pixelEntry = "PS";
        std::string vertexProfile = "vs_5_0";
        std::string pixelProfile = "ps_5_0";
        std::string texturePath;
        int textureWidth = 0;
        int textureHeight = 0;
        int textureChannels = 0;
        std::vector<uint8_t> texturePixels;
    };

    struct DrawableDesc
    {
        MeshData mesh;
        MaterialDesc material;
    };

    class UWU_API IDrawable
    {
    public:
        virtual ~IDrawable() = default;

        virtual bool IsReady() const = 0;

        // Record draw commands for this frame.
        // Renderer internals (command list, etc.) are handled by the concrete class.
        virtual void Draw() = 0;

        virtual void Shutdown() = 0;

        virtual void                   SetTransform(const ObjectTransform& t) = 0;
        virtual const ObjectTransform& GetTransform() const = 0;
    };

}
