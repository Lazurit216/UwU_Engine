#pragma once

#include "Engine/Core.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class UWU_API ResourceManager
    {
    public:
        static ResourceManager& Instance();

        template<typename T>
        std::shared_ptr<Resource<T>> Load(const std::string& path)
        {
            if constexpr (std::is_same_v<T, MeshAsset>)
                return LoadMesh(path);
            else if constexpr (std::is_same_v<T, TextureAsset>)
                return LoadTexture(path);
            else if constexpr (std::is_same_v<T, ShaderAsset>)
                return LoadShader(path);
            else
            {
                static_assert(std::is_same_v<T, MeshAsset>
                    || std::is_same_v<T, TextureAsset>
                    || std::is_same_v<T, ShaderAsset>,
                    "Unsupported resource type");
                return nullptr;
            }
        }

        std::shared_ptr<Resource<MeshAsset>> LoadMesh(const std::string& path);
        std::shared_ptr<Resource<TextureAsset>> LoadTexture(const std::string& path);
        std::shared_ptr<Resource<ShaderAsset>> LoadShader(const std::string& path);

        void Clear();

    private:
        ResourceManager() = default;

        template<typename T, typename Loader>
        std::shared_ptr<Resource<T>> LoadWithCache(
            const std::string& path,
            std::unordered_map<std::string, std::shared_ptr<Resource<T>>>& cache,
            Loader&& loader,
            const char* typeName)
        {
            const std::string normalized = NormalizePath(path);
            if (normalized.empty())
                return nullptr;

            auto it = cache.find(normalized);
            if (it != cache.end())
            {
                UWU_ENGINE_INFO("[ResourceManager] Reusing {} '{}'", typeName, normalized);
                return it->second;
            }

            auto resource = std::make_shared<Resource<T>>(normalized);
            std::string error;
            if (!std::filesystem::exists(normalized))
            {
                error = "file does not exist";
                resource->MarkFailed(error);
                cache[normalized] = resource;
                UWU_ENGINE_WARN("[ResourceManager] {} '{}' failed: {}", typeName, normalized, error);
                return resource;
            }

            if (loader(normalized, resource->GetData(), error))
            {
                resource->MarkLoaded(std::filesystem::last_write_time(normalized));
                UWU_ENGINE_INFO("[ResourceManager] Loaded {} '{}'", typeName, normalized);
            }
            else
            {
                resource->MarkFailed(error);
                UWU_ENGINE_WARN("[ResourceManager] {} '{}' failed: {}", typeName, normalized, error);
            }

            cache[normalized] = resource;
            return resource;
        }

        std::string NormalizePath(const std::string& path) const;

        std::unordered_map<std::string, std::shared_ptr<Resource<MeshAsset>>> m_meshCache;
        std::unordered_map<std::string, std::shared_ptr<Resource<TextureAsset>>> m_textureCache;
        std::unordered_map<std::string, std::shared_ptr<Resource<ShaderAsset>>> m_shaderCache;
    };
}
