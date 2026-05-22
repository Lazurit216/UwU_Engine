#pragma once

#include "Engine/ECS/Components.h"
#include "Engine/ECS/SpatialGrid.h"
#include "Engine/ECS/World.h"
#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{
    struct CameraViewData
    {
        bool enabled = false;
        TransformComponent transform;
        CameraComponent camera;
        SpatialBounds2D visibleBounds;
        std::array<float, 16> viewProjection = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
    };

    UWU_API CameraViewData BuildCameraView(World& world, IRenderer* renderer);
}
