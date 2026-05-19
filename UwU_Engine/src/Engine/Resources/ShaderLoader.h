#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class ShaderLoader
    {
    public:
        static bool Load(const std::string& path, ShaderAsset& out, std::string& error);
    };
}
