#include "uwupch.h"
#include "MeshGeometry.h"

namespace UwU_Engine
{
    namespace
    {
        constexpr float kPi = 3.14159265358979323846f;

        Vertex MakeVertex(
            float x,
            float y,
            float z,
            float nx,
            float ny,
            float nz,
            float u,
            float v,
            Color4 color)
        {
            Vertex vertex{};
            vertex.position[0] = x;
            vertex.position[1] = y;
            vertex.position[2] = z;
            vertex.normal[0] = nx;
            vertex.normal[1] = ny;
            vertex.normal[2] = nz;
            vertex.uv[0] = u;
            vertex.uv[1] = v;
            vertex.color[0] = color.r;
            vertex.color[1] = color.g;
            vertex.color[2] = color.b;
            return vertex;
        }

        void AddBoxFace(
            MeshData& mesh,
            Color4 color,
            const std::array<std::array<float, 3>, 4>& corners,
            const std::array<float, 3>& normal)
        {
            const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back(MakeVertex(corners[0][0], corners[0][1], corners[0][2], normal[0], normal[1], normal[2], 0.0f, 1.0f, color));
            mesh.vertices.push_back(MakeVertex(corners[1][0], corners[1][1], corners[1][2], normal[0], normal[1], normal[2], 0.0f, 0.0f, color));
            mesh.vertices.push_back(MakeVertex(corners[2][0], corners[2][1], corners[2][2], normal[0], normal[1], normal[2], 1.0f, 0.0f, color));
            mesh.vertices.push_back(MakeVertex(corners[3][0], corners[3][1], corners[3][2], normal[0], normal[1], normal[2], 1.0f, 1.0f, color));
            mesh.indices.insert(mesh.indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
        }
    }

    MeshData MeshFactory::CreatePrimitive(PrimitiveType primitive, Color4 color)
    {
        switch (primitive)
        {
        case PrimitiveType::Triangle: return CreateTriangle(1.0f, color);
        case PrimitiveType::Quad:     return CreateQuad(1.0f, 1.0f, color);
        case PrimitiveType::Cube:
        case PrimitiveType::Box:      return CreateBox(1.0f, 1.0f, 1.0f, color);
        case PrimitiveType::Sphere:   return CreateSphere(0.5f, 24, 16, color);
        case PrimitiveType::Plane:    return CreatePlane(1.0f, 1.0f, color);
        default:
            MeshData mesh;
            mesh.primitive = primitive;
            return mesh;
        }
    }

    MeshData MeshFactory::CreateTriangle(float size, Color4 color)
    {
        const float halfSide = size * 0.5f;
        const float bottomY = -size * 0.2886751346f;
        const float topY = size * 0.5773502692f;

        MeshData mesh;
        mesh.primitive = PrimitiveType::Triangle;
        mesh.vertices = {
            MakeVertex(0.0f, topY, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, color),
            MakeVertex(halfSide, bottomY, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, color),
            MakeVertex(-halfSide, bottomY, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, color),
        };
        mesh.indices = { 0, 1, 2 };
        return mesh;
    }

    MeshData MeshFactory::CreateQuad(float width, float height, Color4 color)
    {
        const float hx = width * 0.5f;
        const float hy = height * 0.5f;

        MeshData mesh;
        mesh.primitive = PrimitiveType::Quad;
        mesh.vertices = {
            MakeVertex(-hx, -hy, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, color),
            MakeVertex(-hx,  hy, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, color),
            MakeVertex( hx,  hy, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, color),
            MakeVertex( hx, -hy, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, color),
        };
        mesh.indices = { 0, 1, 2, 0, 2, 3 };
        return mesh;
    }

    MeshData MeshFactory::CreatePlane(float width, float depth, Color4 color)
    {
        const float hx = width * 0.5f;
        const float hz = depth * 0.5f;

        MeshData mesh;
        mesh.primitive = PrimitiveType::Plane;
        mesh.vertices = {
            MakeVertex(-hx, 0.0f, -hz, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, color),
            MakeVertex(-hx, 0.0f,  hz, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, color),
            MakeVertex( hx, 0.0f,  hz, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, color),
            MakeVertex( hx, 0.0f, -hz, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, color),
        };
        mesh.indices = { 0, 1, 2, 0, 2, 3 };
        return mesh;
    }

    MeshData MeshFactory::CreateBox(float width, float height, float depth, Color4 color)
    {
        const float hx = width * 0.5f;
        const float hy = height * 0.5f;
        const float hz = depth * 0.5f;

        MeshData mesh;
        mesh.primitive = PrimitiveType::Box;

        AddBoxFace(mesh, color, { {
            {{ -hx, -hy, -hz }}, {{ -hx,  hy, -hz }}, {{  hx,  hy, -hz }}, {{  hx, -hy, -hz }}
        } }, { 0.0f, 0.0f, -1.0f });

        AddBoxFace(mesh, color, { {
            {{  hx, -hy,  hz }}, {{  hx,  hy,  hz }}, {{ -hx,  hy,  hz }}, {{ -hx, -hy,  hz }}
        } }, { 0.0f, 0.0f, 1.0f });

        AddBoxFace(mesh, color, { {
            {{ -hx, -hy,  hz }}, {{ -hx,  hy,  hz }}, {{ -hx,  hy, -hz }}, {{ -hx, -hy, -hz }}
        } }, { -1.0f, 0.0f, 0.0f });

        AddBoxFace(mesh, color, { {
            {{  hx, -hy, -hz }}, {{  hx,  hy, -hz }}, {{  hx,  hy,  hz }}, {{  hx, -hy,  hz }}
        } }, { 1.0f, 0.0f, 0.0f });

        AddBoxFace(mesh, color, { {
            {{ -hx,  hy, -hz }}, {{ -hx,  hy,  hz }}, {{  hx,  hy,  hz }}, {{  hx,  hy, -hz }}
        } }, { 0.0f, 1.0f, 0.0f });

        AddBoxFace(mesh, color, { {
            {{ -hx, -hy,  hz }}, {{ -hx, -hy, -hz }}, {{  hx, -hy, -hz }}, {{  hx, -hy,  hz }}
        } }, { 0.0f, -1.0f, 0.0f });

        return mesh;
    }

    MeshData MeshFactory::CreateSphere(float radius, uint32_t slices, uint32_t stacks, Color4 color)
    {
        slices = (std::max)(slices, 3u);
        stacks = (std::max)(stacks, 2u);

        MeshData mesh;
        mesh.primitive = PrimitiveType::Sphere;

        for (uint32_t stack = 0; stack <= stacks; ++stack)
        {
            const float v = static_cast<float>(stack) / static_cast<float>(stacks);
            const float phi = v * kPi;
            const float y = std::cos(phi);
            const float ringRadius = std::sin(phi);

            for (uint32_t slice = 0; slice <= slices; ++slice)
            {
                const float u = static_cast<float>(slice) / static_cast<float>(slices);
                const float theta = u * kPi * 2.0f;
                const float x = std::cos(theta) * ringRadius;
                const float z = std::sin(theta) * ringRadius;
                mesh.vertices.push_back(MakeVertex(
                    x * radius,
                    y * radius,
                    z * radius,
                    x,
                    y,
                    z,
                    u,
                    v,
                    color));
            }
        }

        const uint32_t ring = slices + 1;
        for (uint32_t stack = 0; stack < stacks; ++stack)
        {
            for (uint32_t slice = 0; slice < slices; ++slice)
            {
                const uint32_t a = stack * ring + slice;
                const uint32_t b = a + ring;
                mesh.indices.insert(mesh.indices.end(), { a, b, a + 1, a + 1, b, b + 1 });
            }
        }

        return mesh;
    }

    MeshData MeshFactory::CreateLineList(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        MeshData mesh;
        mesh.primitive = PrimitiveType::Line;
        mesh.vertices = vertices;
        mesh.indices = indices;
        return mesh;
    }

    MeshData MeshFactory::CreateAabbWire(
        float centerX,
        float centerY,
        float centerZ,
        float halfX,
        float halfY,
        float halfZ,
        Color4 color)
    {
        std::vector<Vertex> vertices;
        vertices.reserve(8);

        const float xs[] = { centerX - halfX, centerX + halfX };
        const float ys[] = { centerY - halfY, centerY + halfY };
        const float zs[] = { centerZ - halfZ, centerZ + halfZ };

        for (float z : zs)
            for (float y : ys)
                for (float x : xs)
                    vertices.push_back(MakeVertex(x, y, z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, color));

        const std::vector<uint32_t> indices = {
            0, 1, 2, 3, 0, 2, 1, 3,
            4, 5, 6, 7, 4, 6, 5, 7,
            0, 4, 1, 5, 2, 6, 3, 7
        };
        return CreateLineList(vertices, indices);
    }

    MeshData MeshFactory::CreateSphereWire(
        float centerX,
        float centerY,
        float centerZ,
        float radius,
        Color4 color,
        uint32_t segments)
    {
        segments = (std::max)(segments, 8u);
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        vertices.reserve(static_cast<size_t>(segments) * 3);
        indices.reserve(static_cast<size_t>(segments) * 6);

        auto addRing = [&](int axis)
            {
                const uint32_t base = static_cast<uint32_t>(vertices.size());
                for (uint32_t i = 0; i < segments; ++i)
                {
                    const float angle = static_cast<float>(i) / static_cast<float>(segments) * kPi * 2.0f;
                    const float a = std::cos(angle) * radius;
                    const float b = std::sin(angle) * radius;

                    float x = centerX;
                    float y = centerY;
                    float z = centerZ;
                    if (axis == 0) { y += a; z += b; }
                    if (axis == 1) { x += a; z += b; }
                    if (axis == 2) { x += a; y += b; }
                    vertices.push_back(MakeVertex(x, y, z, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, color));
                }

                for (uint32_t i = 0; i < segments; ++i)
                {
                    indices.push_back(base + i);
                    indices.push_back(base + ((i + 1) % segments));
                }
            };

        addRing(0);
        addRing(1);
        addRing(2);
        return CreateLineList(vertices, indices);
    }
}
