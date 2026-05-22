#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class BinaryResourceCache
    {
    public:
        static std::string MeshCachePath(const std::string& sourcePath);
        static std::string TextureCachePath(const std::string& sourcePath);
        static bool IsFresh(const std::string& sourcePath, const std::string& cachePath);

        static bool LoadMesh(const std::string& cachePath, MeshAsset& out, std::string& error);
        static bool SaveMesh(const std::string& cachePath, const MeshAsset& asset, std::string& error);

        static bool LoadTexture(const std::string& cachePath, TextureAsset& out, std::string& error);
        static bool SaveTexture(const std::string& cachePath, const TextureAsset& asset, std::string& error);
    };
}
