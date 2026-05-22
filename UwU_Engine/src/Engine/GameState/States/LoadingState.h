#pragma once

#include "Engine/GameState/IGameState.h"
#include "Engine/Resources/Resource.h"
#include "Engine/Resources/ResourceTypes.h"

namespace UwU_Engine
{
    class UWU_API LoadingState : public IGameState
    {
    public:
        using StateFactory = std::function<std::shared_ptr<IGameState>()>;

        struct ScenePreloadDesc
        {
            std::string scenePath;
            std::wstring fallbackShaderPath;
        };

        explicit LoadingState(StateFactory nextFactory,
            float minimumDisplaySeconds = 0.0f);

        LoadingState(StateFactory nextFactory,
            ScenePreloadDesc preloadDesc,
            float minimumDisplaySeconds = 0.0f);

        void OnEnter(const StateContext& ctx) override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void Render(const StateContext& ctx) override;
        std::string Name() const override { return "Loading"; }

    private:
        void StartPreload();
        void PreloadSceneResources(const std::string& scenePath);
        void UpdateDependentResources();
        void TrackResource(const std::shared_ptr<IResource>& resource);
        void QueueMesh(const std::string& path);
        void QueueTexture(const std::string& path);
        void QueueShader(const std::string& path);
        void QueueMaterial(const std::string& path);
        bool IsLoadingComplete() const;
        float GetProgress() const;

        StateFactory m_nextFactory;
        ScenePreloadDesc m_preloadDesc;
        float m_minimumDisplaySeconds = 0.0f;
        float m_elapsed = 0.0f;
        float m_lastLoggedProgress = -1.0f;
        bool m_preloadStarted = false;

        std::vector<std::shared_ptr<IResource>> m_trackedResources;
        std::vector<std::shared_ptr<Resource<MeshAsset>>> m_meshResources;
        std::vector<std::shared_ptr<Resource<MaterialAsset>>> m_materialResources;
        std::unordered_set<std::string> m_meshPaths;
        std::unordered_set<std::string> m_texturePaths;
        std::unordered_set<std::string> m_shaderPaths;
        std::unordered_set<std::string> m_materialPaths;
    };
}
