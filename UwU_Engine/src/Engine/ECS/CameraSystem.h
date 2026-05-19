#pragma once

#include "Engine/ECS/Entity.h"
#include "Engine/ECS/System.h"

namespace UwU_Engine
{
    class InputManager;

    class UWU_API CameraControlSystem final : public ISystem
    {
    public:
        void Update(World& world, float dt) override;
        void Update(World& world, InputManager* input, float dt);

        EntityId FindPrimaryCamera(World& world) const;
    };
}
