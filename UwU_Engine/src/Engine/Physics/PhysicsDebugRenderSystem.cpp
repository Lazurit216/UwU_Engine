#include "uwupch.h"
#include "PhysicsDebugRenderSystem.h"

#include "Engine/ECS/TransformHierarchy.h"
#include "Engine/Renderer/MeshGeometry.h"

namespace UwU_Engine
{
    void PhysicsDebugRenderSystem::Render(World& world, IRenderer* renderer)
    {
        if (!m_visible || !renderer || !renderer->IsReady())
            return;

        const CameraViewData camera = BuildCameraView(world, renderer);
        std::unordered_set<EntityId> aliveDebugEntities;

        world.ForEach<TransformComponent, ColliderComponent>(
            [this, renderer, &world, &camera, &aliveDebugEntities](EntityId entity, TransformComponent& transform, ColliderComponent& collider)
            {
                aliveDebugEntities.insert(entity);
                const TransformComponent worldTransform = BuildWorldTransform(world, entity, transform);

                if (!EnsureDrawable(entity, worldTransform, collider, renderer))
                    return;

                auto it = m_drawables.find(entity);
                if (it == m_drawables.end() || !it->second.drawable || !it->second.drawable->IsReady())
                    return;

                it->second.drawable->SetTransform(BuildTransform(worldTransform, collider, camera));
                it->second.drawable->Draw();
            });

        for (auto it = m_drawables.begin(); it != m_drawables.end();)
        {
            if (aliveDebugEntities.find(it->first) != aliveDebugEntities.end())
            {
                ++it;
                continue;
            }

            if (it->second.drawable)
                it->second.drawable->Shutdown();
            it = m_drawables.erase(it);
        }
    }

    void PhysicsDebugRenderSystem::Shutdown()
    {
        for (auto& [_, debugDrawable] : m_drawables)
        {
            if (debugDrawable.drawable)
                debugDrawable.drawable->Shutdown();
        }
        m_drawables.clear();
    }

    bool PhysicsDebugRenderSystem::EnsureDrawable(
        EntityId entity,
        const TransformComponent& transform,
        const ColliderComponent& collider,
        IRenderer* renderer)
    {
        const Vector3 scaledHalfExtents{
            collider.halfExtents.x * std::abs(transform.scaleX),
            collider.halfExtents.y * std::abs(transform.scaleY),
            collider.halfExtents.z * std::abs(transform.scaleZ)
        };
        const float radius = collider.type == ColliderType::Sphere
            ? PhysicsSystem::ComputeWorldSphereRadius(transform, collider)
            : 0.0f;

        auto it = m_drawables.find(entity);
        if (it != m_drawables.end()
            && it->second.drawable
            && it->second.type == collider.type
            && std::abs(it->second.halfExtents.x - scaledHalfExtents.x) < 0.0001f
            && std::abs(it->second.halfExtents.y - scaledHalfExtents.y) < 0.0001f
            && std::abs(it->second.halfExtents.z - scaledHalfExtents.z) < 0.0001f
            && std::abs(it->second.radius - radius) < 0.0001f)
        {
            return it->second.drawable->IsReady();
        }

        if (it != m_drawables.end() && it->second.drawable)
            it->second.drawable->Shutdown();

        const Color4 color = collider.isTrigger
            ? Color4{ 1.0f, 0.65f, 0.10f, 1.0f }
            : collider.type == ColliderType::Sphere
                ? Color4{ 0.20f, 0.95f, 1.0f, 1.0f }
                : Color4{ 0.35f, 1.0f, 0.35f, 1.0f };

        DrawableDesc desc;
        desc.mesh = collider.type == ColliderType::Sphere
            ? MeshFactory::CreateSphereWire(0.0f, 0.0f, 0.0f, radius, color)
            : MeshFactory::CreateAabbWire(0.0f, 0.0f, 0.0f, scaledHalfExtents.x, scaledHalfExtents.y, scaledHalfExtents.z, color);
        desc.material.baseColor = color;
        desc.material.shaderPath = m_shaderPath;

        DebugDrawable debugDrawable;
        debugDrawable.type = collider.type;
        debugDrawable.halfExtents = scaledHalfExtents;
        debugDrawable.radius = radius;
        debugDrawable.drawable = renderer->CreateDrawable(desc);
        if (!debugDrawable.drawable)
            return false;

        m_drawables[entity] = std::move(debugDrawable);
        return true;
    }

    ObjectTransform PhysicsDebugRenderSystem::BuildTransform(
        const TransformComponent& transform,
        const ColliderComponent& collider,
        const CameraViewData& camera) const
    {
        const Vector3 center = PhysicsSystem::ComputeWorldCenter(transform, collider);

        ObjectTransform objectTransform;
        objectTransform.x = center.x;
        objectTransform.y = center.y;
        objectTransform.z = center.z;
        objectTransform.scale = 1.0f;
        objectTransform.scaleX = 1.0f;
        objectTransform.scaleY = 1.0f;
        objectTransform.scaleZ = 1.0f;
        if (collider.type == ColliderType::Box)
        {
            objectTransform.rotationX = transform.rotationX;
            objectTransform.rotationY = transform.rotationY;
            objectTransform.rotationZ = transform.rotationZ;
        }

        if (camera.enabled)
        {
            objectTransform.useViewProjection = true;
            objectTransform.viewProjection = camera.viewProjection;
        }

        return objectTransform;
    }
}
