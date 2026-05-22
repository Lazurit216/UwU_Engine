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
            else if constexpr (std::is_same_v<T, MaterialAsset>)
                return LoadMaterial(path);
            else
            {
                static_assert(std::is_same_v<T, MeshAsset>
                    || std::is_same_v<T, TextureAsset>
                    || std::is_same_v<T, ShaderAsset>
                    || std::is_same_v<T, MaterialAsset>,
                    "Unsupported resource type");
                return nullptr;
            }
        }

        template<typename T>
        std::shared_ptr<Resource<T>> LoadAsync(const std::string& path)
        {
            if constexpr (std::is_same_v<T, MeshAsset>)
                return LoadMeshAsync(path);
            else if constexpr (std::is_same_v<T, TextureAsset>)
                return LoadTextureAsync(path);
            else if constexpr (std::is_same_v<T, ShaderAsset>)
                return LoadShaderAsync(path);
            else if constexpr (std::is_same_v<T, MaterialAsset>)
                return LoadMaterialAsync(path);
            else
            {
                static_assert(std::is_same_v<T, MeshAsset>
                    || std::is_same_v<T, TextureAsset>
                    || std::is_same_v<T, ShaderAsset>
                    || std::is_same_v<T, MaterialAsset>,
                    "Unsupported resource type");
                return nullptr;
            }
        }

        std::shared_ptr<Resource<MeshAsset>> LoadMesh(const std::string& path);
        std::shared_ptr<Resource<TextureAsset>> LoadTexture(const std::string& path);
        std::shared_ptr<Resource<ShaderAsset>> LoadShader(const std::string& path);
        std::shared_ptr<Resource<MaterialAsset>> LoadMaterial(const std::string& path);

        std::shared_ptr<Resource<MeshAsset>> LoadMeshAsync(const std::string& path);
        std::shared_ptr<Resource<TextureAsset>> LoadTextureAsync(const std::string& path);
        std::shared_ptr<Resource<ShaderAsset>> LoadShaderAsync(const std::string& path);
        std::shared_ptr<Resource<MaterialAsset>> LoadMaterialAsync(const std::string& path);

        void WaitForAsyncLoads();
        void Clear();

    private:
        ResourceManager() = default;
        ~ResourceManager();

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

            std::shared_ptr<Resource<T>> resource;
            {
                std::scoped_lock lock(m_cacheMutex);
                auto it = cache.find(normalized);
                if (it != cache.end())
                {
                    UWU_ENGINE_INFO("[ResourceManager] Reusing {} '{}'", typeName, normalized);
                    return it->second;
                }

                resource = std::make_shared<Resource<T>>(normalized);
                resource->MarkLoading();
                cache[normalized] = resource;
            }

            LoadIntoResource(resource, normalized, std::forward<Loader>(loader), typeName);
            return resource;
        }

        template<typename T, typename Loader>
        std::shared_ptr<Resource<T>> LoadWithCacheAsync(
            const std::string& path,
            std::unordered_map<std::string, std::shared_ptr<Resource<T>>>& cache,
            Loader&& loader,
            const char* typeName)
        {
            const std::string normalized = NormalizePath(path);
            if (normalized.empty())
                return nullptr;

            std::shared_ptr<Resource<T>> resource;
            bool shouldStartLoading = false;
            {
                std::scoped_lock lock(m_cacheMutex);
                auto it = cache.find(normalized);
                if (it != cache.end())
                {
                    return it->second;
                }

                resource = std::make_shared<Resource<T>>(normalized);
                resource->MarkLoading();
                cache[normalized] = resource;
                shouldStartLoading = true;
            }

            if (shouldStartLoading)
            {
                using LoaderType = std::decay_t<Loader>;
                LoaderType asyncLoader = std::forward<Loader>(loader);
                std::thread worker(
                    [resource, normalized, asyncLoader = std::move(asyncLoader), typeName]()
                    {
                        LoadIntoResource(resource, normalized, asyncLoader, typeName);
                    });
                {
                    std::scoped_lock lock(m_cacheMutex);
                    m_workers.push_back(std::move(worker));
                }
                UWU_ENGINE_INFO("[ResourceManager] Async loading {} '{}'", typeName, normalized);
            }

            return resource;
        }

        template<typename T, typename Loader>
        static void LoadIntoResource(
            const std::shared_ptr<Resource<T>>& resource,
            const std::string& normalized,
            Loader&& loader,
            const char* typeName)
        {
            std::string error;
            if (!std::filesystem::exists(normalized))
            {
                error = "file does not exist";
                resource->MarkFailed(error);
                UWU_ENGINE_WARN("[ResourceManager] {} '{}' failed: {}", typeName, normalized, error);
                return;
            }

            T data;
            if (loader(normalized, data, error))
            {
                resource->SetDataLoaded(std::move(data), std::filesystem::last_write_time(normalized));
                UWU_ENGINE_INFO("[ResourceManager] Loaded {} '{}'", typeName, normalized);
            }
            else
            {
                resource->MarkFailed(error);
                UWU_ENGINE_WARN("[ResourceManager] {} '{}' failed: {}", typeName, normalized, error);
            }
        }

        std::string NormalizePath(const std::string& path) const;

        std::mutex m_cacheMutex;
        std::unordered_map<std::string, std::shared_ptr<Resource<MeshAsset>>> m_meshCache;
        std::unordered_map<std::string, std::shared_ptr<Resource<TextureAsset>>> m_textureCache;
        std::unordered_map<std::string, std::shared_ptr<Resource<ShaderAsset>>> m_shaderCache;
        std::unordered_map<std::string, std::shared_ptr<Resource<MaterialAsset>>> m_materialCache;
        std::vector<std::thread> m_workers;
    };
}
