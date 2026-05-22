#include "uwupch.h"
#include "CameraSystem.h"

#include "Engine/ECS/Components.h"
#include "Engine/Input/InputManager.h"

namespace UwU_Engine
{
    namespace
    {
        Vector3 TransformDirection(const TransformComponent& transform, const Vector3& direction)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            const DirectX::XMVECTOR rotated = DirectX::XMVector3TransformNormal(
                DirectX::XMVectorSet(direction.x, direction.y, direction.z, 0.0f),
                rotation);

            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, rotated);
            return Normalize(Vector3{ result.x, result.y, result.z });
        }
    }

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

        camera->nearPlane = (std::min)(camera->nearPlane, 0.001f);
        camera->farPlane = (std::max)(camera->farPlane, 1000.0f);

        const float zoom = (std::max)(camera->zoom, 0.01f);
        const float speedMultiplier = input->IsKeyDown(VK_SHIFT) ? 3.0f : 1.0f;
        const float moveStep = camera->moveSpeed * speedMultiplier * dt / zoom;

        if (input->IsMouseDown(MouseButton::Right))
        {
            transform->rotationY += input->GetMouseDeltaX() * camera->rotateSpeed;
            transform->rotationX += input->GetMouseDeltaY() * camera->rotateSpeed;
            transform->rotationX = std::clamp(transform->rotationX, -1.45f, 1.45f);
        }

        const Vector3 right = TransformDirection(*transform, Vector3{ 1.0f, 0.0f, 0.0f });
        const Vector3 up = TransformDirection(*transform, Vector3{ 0.0f, 1.0f, 0.0f });
        const Vector3 forward = TransformDirection(*transform, Vector3{ 0.0f, 0.0f, 1.0f });
        Vector3 move;

        if (input->IsKeyDown('A')) move -= right;
        if (input->IsKeyDown('D')) move += right;
        if (camera->orthographic)
        {
            if (input->IsKeyDown('W')) move += up;
            if (input->IsKeyDown('S')) move -= up;
            if (input->IsKeyDown('E')) move += forward;
            if (input->IsKeyDown('Q')) move -= forward;
        }
        else
        {
            if (input->IsKeyDown('W')) move += forward;
            if (input->IsKeyDown('S')) move -= forward;
            if (input->IsKeyDown('E')) move += up;
            if (input->IsKeyDown('Q')) move -= up;
        }

        if (LengthSquared(move) > 0.0001f)
        {
            move = Normalize(move) * moveStep;
            transform->x += move.x;
            transform->y += move.y;
            transform->z += move.z;
        }

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
