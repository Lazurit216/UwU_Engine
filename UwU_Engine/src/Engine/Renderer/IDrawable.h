#pragma once
// Engine/Renderer/IDrawable.h
// Abstract drawable.  Sandbox and states work with IDrawable* only -
// they never need to see DX12Triangle or any command list.

#include "Engine/Core.h"
#include <array>
#include <string>

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

    struct TriangleVertex
    {
        float position[3];
        float color[3];
    };

    struct TriangleDesc
    {
        std::array<TriangleVertex, 3> vertices = { {
            {{ 0.0f,  0.5f, 0.0f }, { 1.f, 0.f, 0.f }},  // top - red
            {{ 0.5f, -0.5f, 0.0f }, { 0.f, 1.f, 0.f }},  // right - green
            {{-0.5f, -0.5f, 0.0f }, { 0.f, 0.f, 1.f }},  // left - blue
        } };
        std::wstring shaderPath = L"Assets/Shaders/Color.hlsl";
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
