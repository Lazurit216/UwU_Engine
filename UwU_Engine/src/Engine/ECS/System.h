#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/World.h"

namespace UwU_Engine
{
    class UWU_API ISystem
    {
    public:
        virtual ~ISystem() = default;
        virtual void Update(World& world, float dt) = 0;
    };
}

