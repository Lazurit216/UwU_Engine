#pragma once

#include "Engine/ECS/CameraView.h"
#include "Engine/Physics/PhysicsSystem.h"
#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{
    class PhysicsDebugRenderSystem
    {
    public:
        void SetVisible(bool visible) { m_visible = visible; }
        bool IsVisible() const { return m_visible; }
        void ToggleVisible() { m_visible = !m_visible; }

        void SetShaderPath(std::wstring shaderPath) { m_shaderPath = std::move(shaderPath); }
        void Render(World& world, IRenderer* renderer);
        void Shutdown();

    private:
        struct DebugDrawable
        {
            std::unique_ptr<IDrawable> drawable;
            ColliderType type = ColliderType::Box;
            Vector3 halfExtents{ 0.0f, 0.0f, 0.0f };
            float radius = 0.0f;
        };

        bool EnsureDrawable(
            EntityId entity,
            const TransformComponent& transform,
            const ColliderComponent& collider,
            IRenderer* renderer);

        ObjectTransform BuildTransform(
            const TransformComponent& transform,
            const ColliderComponent& collider,
            const CameraViewData& camera) const;

        std::wstring m_shaderPath = L"Assets\\Shaders\\Color.hlsl";
        bool m_visible = true;
        std::unordered_map<EntityId, DebugDrawable> m_drawables;
    };
}
