#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class MeshLoader
    {
    public:
        static bool Load(const std::string& path, MeshAsset& out, std::string& error);
    };
}
