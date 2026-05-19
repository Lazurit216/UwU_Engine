#include "uwupch.h"
#include "CameraSystem.h"

#include "Engine/ECS/Components.h"
#include "Engine/Input/InputManager.h"

namespace UwU_Engine
{
    void CameraControlSystem::Update(World& world, float dt)
    {
        Update(world, nullptr, dt);
    }

    void CameraControlSystem::Update(World& world, InputManager* input, float dt)
    {
        if (!input)
            return;

        EntityId cameraEntity = FindPrimaryCamera(world);
        if (cameraEntity == kInvalidEntity)
            return;

        auto* transform = world.GetComponent<TransformComponent>(cameraEntity);
        auto* camera = world.GetComponent<CameraComponent>(cameraEntity);
        if (!transform || !camera)
            return;

        const float zoom = (std::max)(camera->zoom, 0.01f);
        const float moveStep = camera->moveSpeed * dt / zoom;

        if (input->IsKeyDown('A')) transform->x -= moveStep;
        if (input->IsKeyDown('D')) transform->x += moveStep;
        if (input->IsKeyDown('W')) transform->y += moveStep;
        if (input->IsKeyDown('S')) transform->y -= moveStep;

        const float wheelDelta = input->GetMouseWheelDelta();
        if (wheelDelta != 0.0f)
            camera->zoom += wheelDelta * camera->zoomSpeed * 0.12f;

        camera->zoom = std::clamp(camera->zoom, 0.25f, 4.0f);
    }

    EntityId CameraControlSystem::FindPrimaryCamera(World& world) const
    {
        EntityId result = kInvalidEntity;
        world.ForEach<TransformComponent, CameraComponent>(
            [&result](EntityId entity, TransformComponent& /*transform*/, CameraComponent& camera)
            {
                if (result == kInvalidEntity && camera.primary)
                    result = entity;
            });
        return result;
    }
}
