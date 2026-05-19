#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/World.h"

namespace UwU_Engine
{
    class UWU_API SceneSerializer
    {
    public:
        bool Save(World& world, const std::string& filePath) const;
        bool Load(World& world, const std::string& filePath, const std::wstring& fallbackShaderPath) const;
    };
}
