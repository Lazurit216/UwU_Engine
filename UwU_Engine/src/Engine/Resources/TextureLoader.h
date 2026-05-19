#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class TextureLoader
    {
    public:
        static bool Load(const std::string& path, TextureAsset& out, std::string& error);
    };
}
