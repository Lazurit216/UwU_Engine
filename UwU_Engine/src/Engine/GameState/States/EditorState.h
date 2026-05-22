#pragma once

#include "Engine/GameState/IGameState.h"
#include "Engine/GameState/States/GameplayState.h"

namespace UwU_Engine
{
    class UWU_API EditorState : public IGameState
    {
    public:
        explicit EditorState(std::shared_ptr<GameplayState> gameplayState, std::string scenePath = {});

        void OnEnter(const StateContext& ctx) override;
        void OnExit(const StateContext& ctx) override;
        void OnPause(const StateContext& ctx) override;
        void OnResume(const StateContext& ctx) override;
        bool OnEvent(const StateContext& ctx, Event& e) override;
        void FixedUpdate(const StateContext& ctx, float fixedDt) override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void Render(const StateContext& ctx) override;
        std::string Name() const override { return "Editor"; }

    private:
        void DrawEditorUi();
        void DrawDockspace();
        void DrawToolbar();
        void DrawHierarchy(World& world);
        void DrawEntityNode(World& world, EntityId entity);
        void DrawInspector(World& world);
        void DrawTransform(TransformComponent& transform);
        void DrawComponentButtons(World& world, EntityId entity);
        void DrawAssets();
        void DrawConsole();
        void RefreshAssets();
        void SelectFirstEntity(World& world);
        std::filesystem::path ResolveAssetsRoot(const std::string& scenePath) const;

    private:
        std::shared_ptr<GameplayState> m_gameplayState;
        EntityId m_selectedEntity = kInvalidEntity;
        std::filesystem::path m_assetsRoot;
        std::vector<std::filesystem::path> m_assetEntries;

        bool m_simulationEnabled = true;
        bool m_debugDrawEnabled = true;
        bool m_dockLayoutBuilt = false;
        bool m_showTraceLogs = true;
        bool m_showInfoLogs = true;
        bool m_showWarnLogs = true;
        bool m_showErrorLogs = true;
        bool m_showEventLogs = false;
    };
}
