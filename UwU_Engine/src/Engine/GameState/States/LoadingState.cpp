#include "uwupch.h"
#include "LoadingState.h"

#include "Engine/Resources/ResourceManager.h"

#include <nlohmann/json.hpp>

namespace UwU_Engine
{
    namespace
    {
        using Json = nlohmann::json;

        const Json* FindMember(const Json& object, const char* key)
        {
            if (!object.is_object())
                return nullptr;

            const auto it = object.find(key);
            return it == object.end() ? nullptr : &(*it);
        }

        std::string ReadString(const Json& object, const char* key)
        {
            const Json* value = FindMember(object, key);
            return value && value->is_string() ? value->get<std::string>() : std::string{};
        }

        std::string NarrowPath(const std::wstring& value)
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t ch : value)
                result.push_back(ch >= 0 && ch <= 127 ? static_cast<char>(ch) : '?');
            return result;
        }

        std::string NormalizePath(const std::string& path)
        {
            if (path.empty())
                return {};

            return std::filesystem::path(path).lexically_normal().string();
        }

        std::string ResolveSceneAssetPath(const std::string& scenePath, const std::string& assetPath)
        {
            if (assetPath.empty())
                return {};

            std::filesystem::path path(assetPath);
            if (path.is_absolute() || std::filesystem::exists(path))
                return path.lexically_normal().string();

            std::filesystem::path sceneFile(scenePath);
            sceneFile = sceneFile.lexically_normal();

            std::filesystem::path sourcePrefix;
            for (const auto& part : sceneFile)
            {
                if (part == "Assets")
                    break;

                sourcePrefix /= part;
            }

            if (!sourcePrefix.empty())
            {
                std::filesystem::path sourceAssetPath = sourcePrefix / path;
                if (std::filesystem::exists(sourceAssetPath))
                    return sourceAssetPath.lexically_normal().string();
            }

            std::filesystem::path sceneRelativePath = sceneFile.parent_path() / path;
            if (std::filesystem::exists(sceneRelativePath))
                return sceneRelativePath.lexically_normal().string();

            return path.lexically_normal().string();
        }
    }

    LoadingState::LoadingState(StateFactory nextFactory, float minimumDisplaySeconds)
        : LoadingState(std::move(nextFactory), ScenePreloadDesc{}, minimumDisplaySeconds)
    {
    }

    LoadingState::LoadingState(
        StateFactory nextFactory,
        ScenePreloadDesc preloadDesc,
        float minimumDisplaySeconds)
        : m_nextFactory(std::move(nextFactory))
        , m_preloadDesc(std::move(preloadDesc))
        , m_minimumDisplaySeconds(minimumDisplaySeconds)
    {
    }

    void LoadingState::OnEnter(const StateContext& ctx)
    {
        m_elapsed = 0.0f;
        m_lastLoggedProgress = -1.0f;
        m_preloadStarted = false;
        m_trackedResources.clear();
        m_meshResources.clear();
        m_materialResources.clear();
        m_meshPaths.clear();
        m_texturePaths.clear();
        m_shaderPaths.clear();
        m_materialPaths.clear();

        StartPreload();
    }

    StateTransition LoadingState::Update(const StateContext& ctx, float dt)
    {
        m_elapsed += dt;
        UpdateDependentResources();

        const float progress = GetProgress();
        if (progress - m_lastLoggedProgress >= 0.25f || (progress >= 1.0f && m_lastLoggedProgress < 1.0f))
        {
            UWU_ENGINE_INFO("[LoadingState] Resource loading progress: {:.0f}% ({}/{})",
                progress * 100.0f,
                static_cast<int>(std::round(progress * static_cast<float>(m_trackedResources.size()))),
                m_trackedResources.size());
            m_lastLoggedProgress = progress;
        }

        if (IsLoadingComplete() && m_elapsed >= m_minimumDisplaySeconds)
        {
            UWU_ENGINE_INFO("[LoadingState] Resource loading complete - transitioning");
            return m_nextFactory ? StateTransition::Replace(m_nextFactory()) : StateTransition::None();
        }

        return StateTransition::None();
    }

    void LoadingState::Render(const StateContext& ctx)
    {
    }

    void LoadingState::StartPreload()
    {
        if (m_preloadStarted)
            return;

        m_preloadStarted = true;

        if (!m_preloadDesc.scenePath.empty())
            PreloadSceneResources(m_preloadDesc.scenePath);

        const std::string fallbackShaderPath = NarrowPath(m_preloadDesc.fallbackShaderPath);
        if (!fallbackShaderPath.empty())
            QueueShader(fallbackShaderPath);

        UWU_ENGINE_INFO("[LoadingState] Started resource preload: {} queued resource(s)",
            m_trackedResources.size());
    }

    void LoadingState::PreloadSceneResources(const std::string& scenePath)
    {
        std::ifstream file(scenePath);
        if (!file.is_open())
        {
            UWU_ENGINE_WARN("[LoadingState] Cannot open scene '{}' for preload", scenePath);
            return;
        }

        Json root;
        try
        {
            root = Json::parse(file);
        }
        catch (const std::exception& e)
        {
            UWU_ENGINE_WARN("[LoadingState] Cannot parse scene '{}' for preload: {}", scenePath, e.what());
            return;
        }

        const Json* entities = FindMember(root, "entities");
        if (!entities || !entities->is_array())
            return;

        for (const Json& entity : *entities)
        {
            const Json* meshRenderer = FindMember(entity, "meshRenderer");
            if (!meshRenderer || !meshRenderer->is_object())
                continue;

            QueueMesh(ResolveSceneAssetPath(scenePath, ReadString(*meshRenderer, "meshPath")));
            QueueMaterial(ResolveSceneAssetPath(scenePath, ReadString(*meshRenderer, "materialPath")));
            QueueTexture(ResolveSceneAssetPath(scenePath, ReadString(*meshRenderer, "texturePath")));

            const std::string shaderPath = ReadString(*meshRenderer, "shaderPath");
            if (!shaderPath.empty())
                QueueShader(ResolveSceneAssetPath(scenePath, shaderPath));
        }
    }

    void LoadingState::UpdateDependentResources()
    {
        for (const auto& meshResource : m_meshResources)
        {
            if (!meshResource || !meshResource->IsLoaded())
                continue;

            const MeshMaterialData& material = meshResource->GetData().material;
            if (material.hasDiffuseTexture)
                QueueTexture(material.diffuseTexturePath);
        }

        for (const auto& materialResource : m_materialResources)
        {
            if (!materialResource || !materialResource->IsLoaded())
                continue;

            const MaterialAsset& materialAsset = materialResource->GetData();
            if (materialAsset.hasTexturePath)
                QueueTexture(materialAsset.material.texturePath);

            if (materialAsset.hasShaderPath)
                QueueShader(NarrowPath(materialAsset.material.shaderPath));
        }
    }

    void LoadingState::TrackResource(const std::shared_ptr<IResource>& resource)
    {
        if (resource)
            m_trackedResources.push_back(resource);
    }

    void LoadingState::QueueMesh(const std::string& path)
    {
        const std::string normalized = NormalizePath(path);
        if (normalized.empty() || !m_meshPaths.insert(normalized).second)
            return;

        auto resource = ResourceManager::Instance().LoadAsync<MeshAsset>(normalized);
        if (resource)
        {
            m_meshResources.push_back(resource);
            TrackResource(resource);
        }
    }

    void LoadingState::QueueTexture(const std::string& path)
    {
        const std::string normalized = NormalizePath(path);
        if (normalized.empty() || !m_texturePaths.insert(normalized).second)
            return;

        TrackResource(ResourceManager::Instance().LoadAsync<TextureAsset>(normalized));
    }

    void LoadingState::QueueShader(const std::string& path)
    {
        const std::string normalized = NormalizePath(path);
        if (normalized.empty() || !m_shaderPaths.insert(normalized).second)
            return;

        TrackResource(ResourceManager::Instance().LoadAsync<ShaderAsset>(normalized));
    }

    void LoadingState::QueueMaterial(const std::string& path)
    {
        const std::string normalized = NormalizePath(path);
        if (normalized.empty() || !m_materialPaths.insert(normalized).second)
            return;

        auto resource = ResourceManager::Instance().LoadAsync<MaterialAsset>(normalized);
        if (resource)
        {
            m_materialResources.push_back(resource);
            TrackResource(resource);
        }
    }

    bool LoadingState::IsLoadingComplete() const
    {
        for (const auto& resource : m_trackedResources)
        {
            if (!resource)
                continue;

            const ResourceState state = resource->GetState();
            if (state == ResourceState::Unloaded || state == ResourceState::Loading)
                return false;
        }

        return true;
    }

    float LoadingState::GetProgress() const
    {
        if (m_trackedResources.empty())
            return 1.0f;

        size_t completed = 0;
        for (const auto& resource : m_trackedResources)
        {
            if (!resource)
            {
                ++completed;
                continue;
            }

            const ResourceState state = resource->GetState();
            if (state != ResourceState::Unloaded && state != ResourceState::Loading)
                ++completed;
        }

        return static_cast<float>(completed) / static_cast<float>(m_trackedResources.size());
    }
}
