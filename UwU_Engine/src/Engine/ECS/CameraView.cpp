#include "uwupch.h"
#include "CameraView.h"

namespace UwU_Engine
{
    namespace
    {
        Vector3 CameraForward(const TransformComponent& transform)
        {
            const Matrix4 rotation = RotationMatrix(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return Normalize(TransformDirection(rotation, Vector3{ 0.0f, 0.0f, 1.0f }));
        }

        Vector3 CameraUp(const TransformComponent& transform)
        {
            const Matrix4 rotation = RotationMatrix(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return Normalize(TransformDirection(rotation, Vector3{ 0.0f, 1.0f, 0.0f }));
        }

        void CopyMatrixToRowMajorArray(const Matrix4& matrix, std::array<float, 16>& out)
        {
            for (int row = 0; row < 4; ++row)
            {
                for (int column = 0; column < 4; ++column)
                    out[row * 4 + column] = matrix[column][row];
            }
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
                const Vector3 eye{ transform.x, transform.y, transform.z };
                const Matrix4 viewMatrix = glm::lookAtLH(
                    eye,
                    eye + CameraForward(transform),
                    CameraUp(transform));

                const Matrix4 projectionMatrix = camera.orthographic
                    ? glm::orthoLH_ZO(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane)
                    : glm::perspectiveLH_ZO(camera.fovYRadians, aspect, nearPlane, farPlane);

                CopyMatrixToRowMajorArray(glm::transpose(projectionMatrix * viewMatrix), view.viewProjection);
            });

        return view;
    }
}
