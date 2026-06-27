#pragma once

#include "Engine/GameState/IGameState.h"
#include "Engine/GameState/States/GameplayState.h"

struct ImVec2;

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
        void DrawEditorUi(const StateContext& ctx);
        void DrawDockspace();
        void DrawToolbar();
        void SaveCurrentScene();
        void TogglePlayMode();
        void DrawLoadSceneMenu();
        void DrawSceneWindow(const StateContext& ctx, World& world);
        void DrawSelectedGizmo(const StateContext& ctx, World& world, const ImVec2& scenePos, const ImVec2& sceneSize);
        void DrawHierarchy(World& world);
        void DrawEntityNode(World& world, EntityId entity);
        void DrawInspector(World& world);
        void DrawTransform(TransformComponent& transform);
        void DrawComponentButtons(World& world, EntityId entity);
        void DrawHierarchyComponent(World& world, EntityId entity, HierarchyComponent& hierarchy);
        void DrawMeshAssetPicker(EntityId entity, MeshRendererComponent& mesh);
        void DrawMaterialAssetPicker(EntityId entity, MeshRendererComponent& mesh);
        void AddDefaultMeshRenderer(World& world, EntityId entity);
        void ApplyMeshRendererColor(EntityId entity, MeshRendererComponent& mesh);
        void ResetMeshRendererDrawable(MeshRendererComponent& mesh);
        void SetMeshResourcePath(EntityId entity, MeshRendererComponent& mesh, const std::filesystem::path& path);
        void SetMaterialResourcePath(EntityId entity, MeshRendererComponent& mesh, const std::filesystem::path& path);
        void SetEntityParent(World& world, EntityId entity, EntityId parent);
        void RemoveHierarchyComponent(World& world, EntityId entity);
        void DestroyEntityWithChildren(World& world, EntityId entity);
        void EnsureTransform(World& world, EntityId entity);
        std::wstring FindFallbackShaderPath(World& world) const;
        bool IsDescendantOf(const World& world, EntityId entity, EntityId possibleAncestor) const;
        void QueueDestroyEntity(EntityId entity);
        void QueueRemoveMeshRenderer(EntityId entity);
        void QueueResetMeshRendererDrawable(EntityId entity);
        void ApplyPendingEditorOperations(World& world);
        void UpdateStatistics(float dt);
        void DrawAssets();
        void DrawConsole();
        void DrawStatistics(World& world);
        void DrawAssetDirectory(const std::filesystem::path& directory);
        void DrawAssetFile(const std::filesystem::path& path);
        void RefreshAssets();
        void QueueSceneLoad(const std::filesystem::path& path);
        void ApplyPendingSceneLoad();
        void OpenAssetFile(const std::filesystem::path& path) const;
        void SelectFirstEntity(World& world);
        std::filesystem::path ResolveAssetsRoot(const std::string& scenePath) const;

    private:
        std::shared_ptr<GameplayState> m_gameplayState;
        EntityId m_selectedEntity = kInvalidEntity;
        std::filesystem::path m_assetsRoot;
        std::filesystem::path m_currentScenePath;
        std::filesystem::path m_playSnapshotPath;
        std::filesystem::path m_pendingSceneLoadPath;
        std::vector<std::filesystem::path> m_assetEntries;

        bool m_simulationEnabled = false;
        bool m_pendingSceneLoad = false;
        bool m_restoreEditAfterSceneLoad = false;
        bool m_debugDrawEnabled = true;
        bool m_dockLayoutBuilt = false;
        uint32_t m_sceneDockNodeId = 0;
        bool m_showTraceLogs = true;
        bool m_showInfoLogs = true;
        bool m_showWarnLogs = true;
        bool m_showErrorLogs = true;
        bool m_showEventLogs = false;
        int m_gizmoOperation = 0;
        int m_gizmoMode = 0;
        bool m_gizmoSnap = false;
        float m_gizmoSnapValues[3] = { 0.1f, 0.1f, 0.1f };
        float m_lastFrameTimeMs = 0.0f;
        float m_averageFps = 0.0f;
        float m_statsTimer = 0.0f;
        int m_statsFrameCount = 0;
        std::vector<EntityId> m_pendingEntityDeletes;
        std::vector<EntityId> m_pendingMeshRendererRemovals;
        std::vector<EntityId> m_pendingMeshRendererDrawableResets;
    };
}
