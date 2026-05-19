#include "uwupch.h"
#include "ResourceManager.h"

#include "Engine/Resources/MeshLoader.h"
#include "Engine/Resources/ShaderLoader.h"
#include "Engine/Resources/TextureLoader.h"

namespace UwU_Engine
{
    ResourceManager& ResourceManager::Instance()
    {
        static ResourceManager manager;
        return manager;
    }

    std::shared_ptr<Resource<MeshAsset>> ResourceManager::LoadMesh(const std::string& path)
    {
        return LoadWithCache<MeshAsset>(
            path,
            m_meshCache,
            [](const std::string& normalizedPath, MeshAsset& out, std::string& error)
            {
                return MeshLoader::Load(normalizedPath, out, error);
            },
            "mesh");
    }

    std::shared_ptr<Resource<TextureAsset>> ResourceManager::LoadTexture(const std::string& path)
    {
        return LoadWithCache<TextureAsset>(
            path,
            m_textureCache,
            [](const std::string& normalizedPath, TextureAsset& out, std::string& error)
            {
                return TextureLoader::Load(normalizedPath, out, error);
            },
            "texture");
    }

    std::shared_ptr<Resource<ShaderAsset>> ResourceManager::LoadShader(const std::string& path)
    {
        return LoadWithCache<ShaderAsset>(
            path,
            m_shaderCache,
            [](const std::string& normalizedPath, ShaderAsset& out, std::string& error)
            {
                return ShaderLoader::Load(normalizedPath, out, error);
            },
            "shader");
    }

    void ResourceManager::Clear()
    {
        m_meshCache.clear();
        m_textureCache.clear();
        m_shaderCache.clear();
    }

    std::string ResourceManager::NormalizePath(const std::string& path) const
    {
        if (path.empty())
            return {};

        std::filesystem::path fsPath(path);
        fsPath = fsPath.lexically_normal();
        return fsPath.string();
    }
}
