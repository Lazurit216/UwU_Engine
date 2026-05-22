#pragma once

#include "Engine/Renderer/IDrawable.h"

namespace UwU_Engine
{
    class UWU_API MeshFactory
    {
    public:
        static MeshData CreatePrimitive(PrimitiveType primitive, Color4 color = {});

        static MeshData CreateTriangle(float size = 1.0f, Color4 color = {});
        static MeshData CreateQuad(float width = 1.0f, float height = 1.0f, Color4 color = {});
        static MeshData CreatePlane(float width = 1.0f, float depth = 1.0f, Color4 color = {});
        static MeshData CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f, Color4 color = {});
        static MeshData CreateSphere(float radius = 0.5f, uint32_t slices = 24, uint32_t stacks = 16, Color4 color = {});

        static MeshData CreateLineList(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        static MeshData CreateAabbWire(
            float centerX,
            float centerY,
            float centerZ,
            float halfX,
            float halfY,
            float halfZ,
            Color4 color = {});
        static MeshData CreateSphereWire(
            float centerX,
            float centerY,
            float centerZ,
            float radius,
            Color4 color = {},
            uint32_t segments = 32);
    };
}
