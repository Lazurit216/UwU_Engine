#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Components.h"
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

            world.ForEach<TransformComponent, MeshRendererComponent>(
                [this, &world, renderer](EntityId entity, TransformComponent& transform, MeshRendererComponent& mesh)
                {
                    if (!EnsureDrawable(world, entity, mesh, renderer))
                        return;

                    ObjectTransform objectTransform = ToObjectTransform(world, entity, transform);
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
            if (mesh.mesh.primitive != PrimitiveType::Triangle)
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
            local.rotation = transform.rotationZ;
            local.scale = (transform.scaleX + transform.scaleY) * 0.5f;

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
            parent.rotation = parentTransform->rotationZ;
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
            worldTransform.scale = parent.scale * local.scale;
            return worldTransform;
        }
    };
}
