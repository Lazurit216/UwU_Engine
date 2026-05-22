#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class MaterialLoader
    {
    public:
        static bool Load(const std::string& path, MaterialAsset& out, std::string& error);
    };
}
