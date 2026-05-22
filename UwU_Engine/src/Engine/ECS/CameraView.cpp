#include "uwupch.h"
#include "CameraView.h"

namespace UwU_Engine
{
    namespace
    {
        DirectX::XMVECTOR CameraForward(const TransformComponent& transform)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        }

        DirectX::XMVECTOR CameraUp(const TransformComponent& transform)
        {
            const DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return DirectX::XMVector3TransformNormal(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);
        }
    }

    CameraViewData BuildCameraView(World& world, IRenderer* renderer)
    {
        CameraViewData view;
        const float width = renderer ? static_cast<float>(renderer->GetWidth()) : 1.0f;
        const float height = renderer ? static_cast<float>(renderer->GetHeight()) : 1.0f;
        const float aspect = height > 0.0f ? width / height : 1.0f;

        world.ForEach<TransformComponent, CameraComponent>(
            [&view, aspect](EntityId /*entity*/, TransformComponent& transform, CameraComponent& camera)
            {
                if (view.enabled || !camera.primary)
                    return;

                view.enabled = true;
                view.transform = transform;
                view.camera = camera;

                const float zoom = (std::max)(camera.zoom, 0.01f);
                const float halfWidth = (std::max)(camera.viewHalfWidth, camera.viewHalfHeight * aspect) / zoom;
                const float halfHeight = camera.viewHalfHeight / zoom;
                view.visibleBounds = SpatialBounds2D{
                    transform.x - halfWidth,
                    transform.y - halfHeight,
                    transform.x + halfWidth,
                    transform.y + halfHeight
                };

                const float nearPlane = (std::max)(camera.nearPlane, 0.001f);
                const float farPlane = (std::max)(camera.farPlane, nearPlane + 1.0f);
                const DirectX::XMVECTOR eye = DirectX::XMVectorSet(transform.x, transform.y, transform.z, 1.0f);
                const DirectX::XMMATRIX viewMatrix = DirectX::XMMatrixLookToLH(
                    eye,
                    CameraForward(transform),
                    CameraUp(transform));

                const DirectX::XMMATRIX projectionMatrix = camera.orthographic
                    ? DirectX::XMMatrixOrthographicLH(halfWidth * 2.0f, halfHeight * 2.0f, nearPlane, farPlane)
                    : DirectX::XMMatrixPerspectiveFovLH(camera.fovYRadians, aspect, nearPlane, farPlane);

                DirectX::XMFLOAT4X4 matrixData;
                DirectX::XMStoreFloat4x4(&matrixData, viewMatrix * projectionMatrix);
                for (int row = 0; row < 4; ++row)
                {
                    for (int column = 0; column < 4; ++column)
                        view.viewProjection[row * 4 + column] = matrixData.m[row][column];
                }
            });

        return view;
    }
}
