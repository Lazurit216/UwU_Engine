#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Renderer/IDrawable.h"

namespace UwU_Engine
{
    struct TransformComponent
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float rotationZ = 0.0f;

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;
    };

    struct TagComponent
    {
        std::string name;
    };

    struct MeshRendererComponent
    {
        MeshData mesh;
        MaterialDesc material;
        std::string meshResourcePath;

        std::unique_ptr<IDrawable> drawable;
        bool initFailedLogged = false;
        bool unsupportedLogged = false;
        bool textureUnsupportedLogged = false;

        MeshRendererComponent() = default;
        MeshRendererComponent(MeshData meshData, MaterialDesc materialDesc)
            : mesh(std::move(meshData))
            , material(std::move(materialDesc))
        {
        }

        MeshRendererComponent(const MeshRendererComponent&) = delete;
        MeshRendererComponent& operator=(const MeshRendererComponent&) = delete;

        MeshRendererComponent(MeshRendererComponent&&) noexcept = default;

        MeshRendererComponent& operator=(MeshRendererComponent&& other) noexcept
        {
            if (this == &other) return *this;

            if (drawable)
                drawable->Shutdown();

            mesh = std::move(other.mesh);
            material = std::move(other.material);
            drawable = std::move(other.drawable);
            meshResourcePath = std::move(other.meshResourcePath);
            initFailedLogged = other.initFailedLogged;
            unsupportedLogged = other.unsupportedLogged;
            textureUnsupportedLogged = other.textureUnsupportedLogged;
            return *this;
        }

        ~MeshRendererComponent()
        {
            if (drawable)
                drawable->Shutdown();
        }
    };

    struct HierarchyComponent
    {
        EntityId parent = kInvalidEntity;
        std::vector<EntityId> children;
    };

    struct CameraComponent
    {
        bool primary = true;
        float zoom = 1.0f;
        float viewHalfWidth = 1.8f;
        float viewHalfHeight = 1.0f;
        float moveSpeed = 0.8f;
        float zoomSpeed = 1.0f;
    };
}
