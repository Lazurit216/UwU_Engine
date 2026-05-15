#pragma once
// Engine/Renderer/IDrawable.h
// Abstract drawable.  Sandbox and states work with IDrawable* only -
// they never need to see DX12Triangle or any command list.

#include "Engine/Core.h"
#include <string>
#include <vector>

namespace UwU_Engine
{
    //2D world transform (NDC coordinates)
    struct UWU_API ObjectTransform
    {
        float x = 0.f;   // horizontal offset  [-1.8 .. 1.8]
        float y = 0.f;   // vertical offset    [-1.8 .. 1.8]
        float scale = 1.f;   // uniform scale      [ 0.1 ..  5.0]
        float rotation = 0.f;   // radians, CCW
    };

    struct Vertex
    {
        float position[3];
        float color[3];
    };

    enum class PrimitiveType
    {
        Line,
        Triangle,
        Quad,
        Cube,
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
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct MaterialDesc
    {
        Color4 baseColor;
        std::wstring shaderPath = L"Assets/Shaders/Color.hlsl";
    };

    struct DrawableDesc
    {
        MeshData mesh;
        MaterialDesc material;
    };

    class MeshFactory
    {
    public:
        static MeshData CreateTriangle(float size = 1.0f, Color4 color = {})
        {
            const float h = size * 0.5f;

            MeshData mesh;
            mesh.primitive = PrimitiveType::Triangle;
            mesh.vertices = {
                {{ 0.0f,  h, 0.0f }, { color.r, color.g, color.b }},
                {{ h,    -h, 0.0f }, { color.r, color.g, color.b }},
                {{-h,    -h, 0.0f }, { color.r, color.g, color.b }},
            };
            mesh.indices = { 0, 1, 2 };
            return mesh;
        }
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
