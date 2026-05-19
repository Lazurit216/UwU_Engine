#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Components.h"
#include "Engine/ECS/SpatialGrid.h"
#include "Engine/ECS/System.h"
#include "Engine/Renderer/IRenderer.h"
#include "Engine/Resources/ResourceManager.h"

namespace UwU_Engine
{
    class UWU_API RenderSystem final : public ISystem
    {
    private:
        struct CameraView
        {
            bool enabled = false;
            TransformComponent transform;
            CameraComponent camera;
            SpatialBounds2D visibleBounds;
        };

    public:
        void Update(World& /*world*/, float /*dt*/) override {}

        void Render(World& world, IRenderer* renderer)
        {
            if (!renderer || !renderer->IsReady())
                return;

            const CameraView camera = FindActiveCamera(world);
            m_spatialGrid.Clear();

            world.ForEach<TransformComponent, MeshRendererComponent>(
                [this, &world](EntityId entity, TransformComponent& transform, MeshRendererComponent& mesh)
                {
                    const ObjectTransform worldTransform = ToObjectTransform(world, entity, transform);
                    m_spatialGrid.Insert(entity, ComputeBounds(mesh.mesh, worldTransform));
                });

            std::unordered_set<EntityId> visibleEntities;
            if (camera.enabled)
            {
                const auto visible = m_spatialGrid.Query(camera.visibleBounds);
                visibleEntities.insert(visible.begin(), visible.end());
            }

            world.ForEach<TransformComponent, MeshRendererComponent>(
                [this, &world, renderer, &camera, &visibleEntities](EntityId entity, TransformComponent& transform, MeshRendererComponent& mesh)
                {
                    if (camera.enabled && visibleEntities.find(entity) == visibleEntities.end())
                        return;

                    if (!EnsureDrawable(world, entity, mesh, renderer))
                        return;

                    ObjectTransform objectTransform = ToObjectTransform(world, entity, transform);
                    if (camera.enabled)
                        objectTransform = ApplyCamera(objectTransform, camera);

                    mesh.drawable->SetTransform(objectTransform);
                    mesh.drawable->Draw();
                });
        }

        void Shutdown(World& world)
        {
            world.ForEach<MeshRendererComponent>(
                [](EntityId /*entity*/, MeshRendererComponent& mesh)
                {
                    if (mesh.drawable)
                    {
                        mesh.drawable->Shutdown();
                        mesh.drawable.reset();
                    }
                });
        }

    private:
        bool EnsureDrawable(World& world, EntityId entity, MeshRendererComponent& mesh, IRenderer* renderer)
        {
            LoadResources(entity, mesh);

            if (mesh.mesh.primitive != PrimitiveType::Triangle
                && mesh.mesh.primitive != PrimitiveType::CustomMesh)
            {
                if (!mesh.unsupportedLogged)
                {
                    UWU_ENGINE_WARN("[RenderSystem] Entity {} uses unsupported primitive type", entity);
                    mesh.unsupportedLogged = true;
                }
                return false;
            }

            if (mesh.drawable)
                return mesh.drawable->IsReady();

            DrawableDesc desc;
            desc.mesh = mesh.mesh;
            desc.material = mesh.material;

            mesh.drawable = renderer->CreateDrawable(desc);
            if (!mesh.drawable)
            {
                if (!mesh.initFailedLogged)
                {
                    const TagComponent* tag = world.GetComponent<TagComponent>(entity);
                    UWU_ENGINE_ERROR("[RenderSystem] Failed to create drawable for entity {} ({})",
                        entity, tag ? tag->name : "untagged");
                    mesh.initFailedLogged = true;
                }
                return false;
            }

            const TagComponent* tag = world.GetComponent<TagComponent>(entity);
            UWU_ENGINE_INFO("[RenderSystem] Drawable created for entity {} ({})",
                entity, tag ? tag->name : "untagged");
            return true;
        }

        void LoadResources(EntityId entity, MeshRendererComponent& mesh)
        {
            auto& resources = ResourceManager::Instance();

            if (!mesh.meshResourcePath.empty() && !mesh.meshResource)
            {
                mesh.meshResource = resources.Load<MeshAsset>(mesh.meshResourcePath);
                if (mesh.meshResource && mesh.meshResource->IsLoaded())
                {
                    const MeshAsset& meshAsset = mesh.meshResource->GetData();
                    mesh.mesh = meshAsset.mesh;
                    ApplyMeshAssetMaterial(mesh, meshAsset);
                    ApplyMaterialColor(mesh.mesh, mesh.material.baseColor);
                    UWU_ENGINE_INFO("[RenderSystem] Entity {} uses mesh resource '{}'",
                        entity, mesh.meshResourcePath);
                }
                else if (!mesh.meshLoadFailedLogged)
                {
                    UWU_ENGINE_WARN("[RenderSystem] Entity {} mesh resource '{}' is unavailable",
                        entity, mesh.meshResourcePath);
                    mesh.meshLoadFailedLogged = true;
                }
            }

            if (!mesh.material.texturePath.empty() && !mesh.textureResource)
            {
                mesh.textureResource = resources.Load<TextureAsset>(mesh.material.texturePath);
                if (mesh.textureResource && mesh.textureResource->IsLoaded())
                {
                    const auto& texture = mesh.textureResource->GetData().texture;
                    mesh.material.textureWidth = texture.width;
                    mesh.material.textureHeight = texture.height;
                    mesh.material.textureChannels = texture.channels;
                    mesh.material.texturePixels = texture.pixels;
                    UWU_ENGINE_INFO("[RenderSystem] Entity {} uses texture resource '{}'",
                        entity, mesh.material.texturePath);
                }
                else
                {
                    if (!mesh.textureLoadFailedLogged)
                    {
                        UWU_ENGINE_WARN("[RenderSystem] Entity {} texture resource '{}' is unavailable",
                            entity, mesh.material.texturePath);
                        mesh.textureLoadFailedLogged = true;
                    }
                }
            }

            const std::string shaderPath = NarrowPath(mesh.material.shaderPath);
            if (!shaderPath.empty() && !mesh.shaderResource)
            {
                mesh.shaderResource = resources.Load<ShaderAsset>(shaderPath);
                if (!mesh.shaderResource || !mesh.shaderResource->IsLoaded())
                {
                    if (!mesh.shaderLoadFailedLogged)
                    {
                        UWU_ENGINE_WARN("[RenderSystem] Entity {} shader resource '{}' is unavailable",
                            entity, shaderPath);
                        mesh.shaderLoadFailedLogged = true;
                    }
                }
            }
        }

        ObjectTransform ToObjectTransform(World& world, EntityId entity, const TransformComponent& transform) const
        {
            ObjectTransform local;
            local.x = transform.x;
            local.y = transform.y;
            local.rotation = transform.rotationZ;
            local.rotationY = transform.rotationY;
            local.scale = (transform.scaleX + transform.scaleY) * 0.5f;

            return CombineWithParents(world, entity, local, 0);
        }

        void ApplyMaterialColor(MeshData& mesh, const Color4& color) const
        {
            for (Vertex& vertex : mesh.vertices)
            {
                vertex.color[0] *= color.r;
                vertex.color[1] *= color.g;
                vertex.color[2] *= color.b;
            }
        }

        void ApplyMeshAssetMaterial(MeshRendererComponent& renderer, const MeshAsset& asset) const
        {
            if (renderer.material.texturePath.empty() && asset.material.hasDiffuseTexture)
            {
                renderer.material.texturePath = asset.material.diffuseTexturePath;
                UWU_ENGINE_INFO("[RenderSystem] Using mesh material texture '{}'",
                    renderer.material.texturePath);
            }

            if (asset.material.hasDiffuseColor)
            {
                renderer.material.baseColor.r *= asset.material.diffuseColor.r;
                renderer.material.baseColor.g *= asset.material.diffuseColor.g;
                renderer.material.baseColor.b *= asset.material.diffuseColor.b;
                renderer.material.baseColor.a *= asset.material.diffuseColor.a;
            }
        }

        ObjectTransform CombineWithParents(World& world, EntityId entity, ObjectTransform local, int depth) const
        {
            if (depth > 32)
                return local;

            const HierarchyComponent* hierarchy = world.GetComponent<HierarchyComponent>(entity);
            if (!hierarchy || hierarchy->parent == kInvalidEntity || !world.IsAlive(hierarchy->parent))
                return local;

            const TransformComponent* parentTransform = world.GetComponent<TransformComponent>(hierarchy->parent);
            if (!parentTransform)
                return local;

            ObjectTransform parent;
            parent.x = parentTransform->x;
            parent.y = parentTransform->y;
            parent.rotation = parentTransform->rotationZ;
            parent.rotationY = parentTransform->rotationY;
            parent.scale = (parentTransform->scaleX + parentTransform->scaleY) * 0.5f;
            parent = CombineWithParents(world, hierarchy->parent, parent, depth + 1);

            const float scaledX = local.x * parent.scale;
            const float scaledY = local.y * parent.scale;
            const float c = std::cos(parent.rotation);
            const float s = std::sin(parent.rotation);

            ObjectTransform worldTransform;
            worldTransform.x = parent.x + scaledX * c - scaledY * s;
            worldTransform.y = parent.y + scaledX * s + scaledY * c;
            worldTransform.rotation = parent.rotation + local.rotation;
            worldTransform.rotationY = parent.rotationY + local.rotationY;
            worldTransform.scale = parent.scale * local.scale;
            return worldTransform;
        }

        CameraView FindActiveCamera(World& world) const
        {
            CameraView view;

            world.ForEach<TransformComponent, CameraComponent>(
                [&view](EntityId /*entity*/, TransformComponent& transform, CameraComponent& camera)
                {
                    if (view.enabled || !camera.primary)
                        return;

                    view.enabled = true;
                    view.transform = transform;
                    view.camera = camera;

                    const float zoom = (std::max)(camera.zoom, 0.01f);
                    const float halfWidth = camera.viewHalfWidth / zoom;
                    const float halfHeight = camera.viewHalfHeight / zoom;
                    view.visibleBounds = SpatialBounds2D{
                        transform.x - halfWidth,
                        transform.y - halfHeight,
                        transform.x + halfWidth,
                        transform.y + halfHeight
                    };
                });

            return view;
        }

        ObjectTransform ApplyCamera(const ObjectTransform& worldTransform, const CameraView& camera) const
        {
            const float dx = worldTransform.x - camera.transform.x;
            const float dy = worldTransform.y - camera.transform.y;
            const float inverseRotation = -camera.transform.rotationZ;
            const float c = std::cos(inverseRotation);
            const float s = std::sin(inverseRotation);

            ObjectTransform result;
            result.x = (dx * c - dy * s) * camera.camera.zoom;
            result.y = (dx * s + dy * c) * camera.camera.zoom;
            result.rotation = worldTransform.rotation - camera.transform.rotationZ;
            result.rotationY = worldTransform.rotationY;
            result.scale = worldTransform.scale * camera.camera.zoom;
            return result;
        }

        SpatialBounds2D ComputeBounds(const MeshData& mesh, const ObjectTransform& transform) const
        {
            float minX = -0.5f;
            float minY = -0.5f;
            float maxX = 0.5f;
            float maxY = 0.5f;

            if (!mesh.vertices.empty())
            {
                minX = maxX = mesh.vertices[0].position[0];
                minY = maxY = mesh.vertices[0].position[1];

                for (const auto& vertex : mesh.vertices)
                {
                    minX = (std::min)(minX, vertex.position[0]);
                    minY = (std::min)(minY, vertex.position[1]);
                    maxX = (std::max)(maxX, vertex.position[0]);
                    maxY = (std::max)(maxY, vertex.position[1]);
                }
            }

            const float c = std::cos(transform.rotation);
            const float s = std::sin(transform.rotation);
            const float corners[4][2] = {
                { minX, minY },
                { minX, maxY },
                { maxX, minY },
                { maxX, maxY }
            };

            SpatialBounds2D bounds;
            bool first = true;
            for (const auto& corner : corners)
            {
                const float scaledX = corner[0] * transform.scale;
                const float scaledY = corner[1] * transform.scale;
                const float worldX = transform.x + scaledX * c - scaledY * s;
                const float worldY = transform.y + scaledX * s + scaledY * c;

                if (first)
                {
                    bounds.minX = bounds.maxX = worldX;
                    bounds.minY = bounds.maxY = worldY;
                    first = false;
                }
                else
                {
                    bounds.minX = (std::min)(bounds.minX, worldX);
                    bounds.minY = (std::min)(bounds.minY, worldY);
                    bounds.maxX = (std::max)(bounds.maxX, worldX);
                    bounds.maxY = (std::max)(bounds.maxY, worldY);
                }
            }

            return bounds;
        }

        std::string NarrowPath(const std::wstring& value) const
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t ch : value)
                result.push_back(ch >= 0 && ch <= 127 ? static_cast<char>(ch) : '?');
            return result;
        }

        SpatialGrid m_spatialGrid{ 1.0f };
    };
}
