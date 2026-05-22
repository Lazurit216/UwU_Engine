#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Physics/PhysicsTypes.h"
#include "Engine/Renderer/IDrawable.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    struct TransformComponent
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        float scaleZ = 1.0f;

        float rotationX = 0.0f;
        float rotationY = 0.0f;
        float rotationZ = 0.0f;
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
        std::string materialResourcePath;

        std::shared_ptr<Resource<MeshAsset>> meshResource;
        std::shared_ptr<Resource<TextureAsset>> textureResource;
        std::shared_ptr<Resource<ShaderAsset>> shaderResource;
        std::shared_ptr<Resource<MaterialAsset>> materialResource;
        std::unique_ptr<IDrawable> drawable;
        bool initFailedLogged = false;
        bool unsupportedLogged = false;
        bool textureUnsupportedLogged = false;
        bool meshLoadFailedLogged = false;
        bool textureLoadFailedLogged = false;
        bool shaderLoadFailedLogged = false;
        bool materialLoadFailedLogged = false;
        bool meshResourceApplied = false;
        bool textureResourceApplied = false;
        bool shaderResourceApplied = false;
        bool materialResourceApplied = false;
        bool placeholderMeshApplied = false;

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
            meshResourcePath = std::move(other.meshResourcePath);
            materialResourcePath = std::move(other.materialResourcePath);
            meshResource = std::move(other.meshResource);
            textureResource = std::move(other.textureResource);
            shaderResource = std::move(other.shaderResource);
            materialResource = std::move(other.materialResource);
            drawable = std::move(other.drawable);
            initFailedLogged = other.initFailedLogged;
            unsupportedLogged = other.unsupportedLogged;
            textureUnsupportedLogged = other.textureUnsupportedLogged;
            meshLoadFailedLogged = other.meshLoadFailedLogged;
            textureLoadFailedLogged = other.textureLoadFailedLogged;
            shaderLoadFailedLogged = other.shaderLoadFailedLogged;
            materialLoadFailedLogged = other.materialLoadFailedLogged;
            meshResourceApplied = other.meshResourceApplied;
            textureResourceApplied = other.textureResourceApplied;
            shaderResourceApplied = other.shaderResourceApplied;
            materialResourceApplied = other.materialResourceApplied;
            placeholderMeshApplied = other.placeholderMeshApplied;
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
        bool orthographic = true;
        float zoom = 1.0f;
        float viewHalfWidth = 1.8f;
        float viewHalfHeight = 1.0f;
        float fovYRadians = 1.04719755f;
        float nearPlane = 0.001f;
        float farPlane = 1000.0f;
        float moveSpeed = 0.8f;
        float zoomSpeed = 1.0f;
        float rotateSpeed = 0.012f;
    };

    struct RigidbodyComponent
    {
        Vector3 velocity;
        Vector3 acceleration;
        float mass = 1.0f;
        bool useGravity = true;
        bool isStatic = false;
        float linearDamping = 0.0f;
    };

    struct ColliderComponent
    {
        ColliderType type = ColliderType::Box;
        Vector3 halfExtents{ 0.5f, 0.5f, 0.5f };
        float radius = 0.5f;
        Vector3 offset;
        bool isTrigger = false;
        float bounciness = 0.0f;
        float friction = 0.2f;
    };

    struct PlayerControllerComponent
    {
        float moveSpeed = 1.4f;
        float jumpSpeed = 3.0f;
    };
}
