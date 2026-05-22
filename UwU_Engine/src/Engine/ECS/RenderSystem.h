#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/CameraView.h"
#include "Engine/ECS/Components.h"
#include "Engine/ECS/RenderResourceBinder.h"
#include "Engine/ECS/SpatialGrid.h"
#include "Engine/ECS/System.h"
#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{
    class UWU_API RenderSystem final : public ISystem
    {
    public:
        void Update(World& /*world*/, float /*dt*/) override {}

        void Render(World& world, IRenderer* renderer)
        {
            if (!renderer || !renderer->IsReady())
                return;

            const CameraViewData camera = FindActiveCamera(world, renderer);
            m_spatialGrid.Clear();

            world.ForEach<TransformComponent, MeshRendererComponent>(
                [this, &world](EntityId entity, TransformComponent& transform, MeshRendererComponent& mesh)
                {
                    const ObjectTransform worldTransform = ToObjectTransform(world, entity, transform);
                    m_spatialGrid.Insert(entity, ComputeBounds(mesh.mesh, worldTransform));
                });

            std::unordered_set<EntityId> visibleEntities;
            const bool useSpatialCulling = camera.enabled
                && camera.camera.orthographic
                && std::abs(camera.transform.rotationX) < 0.0001f
                && std::abs(camera.transform.rotationY) < 0.0001f
                && std::abs(camera.transform.rotationZ) < 0.0001f;

            if (useSpatialCulling)
            {
                const auto visible = m_spatialGrid.Query(camera.visibleBounds);
                visibleEntities.insert(visible.begin(), visible.end());
            }

            world.ForEach<TransformComponent, MeshRendererComponent>(
                [this, &world, renderer, &camera, useSpatialCulling, &visibleEntities](EntityId entity, TransformComponent& transform, MeshRendererComponent& mesh)
                {
                    if (useSpatialCulling && visibleEntities.find(entity) == visibleEntities.end())
                        return;

                    if (!EnsureDrawable(world, entity, mesh, renderer))
                        return;

                    ObjectTransform objectTransform = ToObjectTransform(world, entity, transform);
                    if (camera.enabled)
                    {
                        objectTransform.useViewProjection = true;
                        objectTransform.viewProjection = camera.viewProjection;
                    }

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
            if (!m_resourceBinder.Update(entity, mesh))
                return false;

            if (mesh.mesh.primitive != PrimitiveType::Line
                && mesh.mesh.primitive != PrimitiveType::Triangle
                && mesh.mesh.primitive != PrimitiveType::Quad
                && mesh.mesh.primitive != PrimitiveType::Cube
                && mesh.mesh.primitive != PrimitiveType::Box
                && mesh.mesh.primitive != PrimitiveType::Sphere
                && mesh.mesh.primitive != PrimitiveType::Plane
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

        ObjectTransform ToObjectTransform(World& world, EntityId entity, const TransformComponent& transform) const
        {
            ObjectTransform local;
            local.x = transform.x;
            local.y = transform.y;
            local.z = transform.z;
            local.rotation = 0.0f;
            local.rotationX = transform.rotationX;
            local.rotationY = transform.rotationY;
            local.rotationZ = transform.rotationZ;
            local.scale = 1.0f;
            local.scaleX = transform.scaleX;
            local.scaleY = transform.scaleY;
            local.scaleZ = transform.scaleZ;

            return CombineWithParents(world, entity, local, 0);
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
            parent.z = parentTransform->z;
            parent.rotation = 0.0f;
            parent.rotationX = parentTransform->rotationX;
            parent.rotationY = parentTransform->rotationY;
            parent.rotationZ = parentTransform->rotationZ;
            parent.scale = 1.0f;
            parent.scaleX = parentTransform->scaleX;
            parent.scaleY = parentTransform->scaleY;
            parent.scaleZ = parentTransform->scaleZ;
            parent = CombineWithParents(world, hierarchy->parent, parent, depth + 1);

            const float parentRotationZ = parent.rotation + parent.rotationZ;
            const float localRotationZ = local.rotation + local.rotationZ;
            const float scaledX = local.x * parent.scale * parent.scaleX;
            const float scaledY = local.y * parent.scale * parent.scaleY;
            const float c = std::cos(parentRotationZ);
            const float s = std::sin(parentRotationZ);

            ObjectTransform worldTransform;
            worldTransform.x = parent.x + scaledX * c - scaledY * s;
            worldTransform.y = parent.y + scaledX * s + scaledY * c;
            worldTransform.z = parent.z + local.z * parent.scale * parent.scaleZ;
            worldTransform.rotation = 0.0f;
            worldTransform.rotationX = parent.rotationX + local.rotationX;
            worldTransform.rotationY = parent.rotationY + local.rotationY;
            worldTransform.rotationZ = parentRotationZ + localRotationZ;
            worldTransform.scale = parent.scale * local.scale;
            worldTransform.scaleX = parent.scaleX * local.scaleX;
            worldTransform.scaleY = parent.scaleY * local.scaleY;
            worldTransform.scaleZ = parent.scaleZ * local.scaleZ;
            return worldTransform;
        }

        CameraViewData FindActiveCamera(World& world, IRenderer* renderer) const
        {
            return BuildCameraView(world, renderer);
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

            const float rotationZ = transform.rotation + transform.rotationZ;
            const float c = std::cos(rotationZ);
            const float s = std::sin(rotationZ);
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
                const float scaledX = corner[0] * transform.scale * transform.scaleX;
                const float scaledY = corner[1] * transform.scale * transform.scaleY;
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

        RenderResourceBinder m_resourceBinder;
        SpatialGrid m_spatialGrid{ 1.0f };
    };
}
