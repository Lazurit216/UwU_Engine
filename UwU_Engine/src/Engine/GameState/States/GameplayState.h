#pragma once
// Prototype gameplay state.
// Logs deltatime each frame (satisfies the "deltatime logging" requirement).
// In a real project this state would own the scene, camera, entities, etc.

#include "Engine/GameState/IGameState.h"
#include "Engine/Renderer/IDrawable.h"

namespace UwU_Engine
{
    using DrawableFactory = std::function<std::unique_ptr<IDrawable>(IRenderer*)>;

    class UWU_API GameplayState : public IGameState
    {
    public:
        using StateFactory = std::function<std::shared_ptr<IGameState>()>;


        explicit GameplayState(StateFactory pauseFactory = nullptr, DrawableFactory drawFactory = nullptr);

        void OnEnter(const StateContext& ctx)  override;
        void OnPause(const StateContext& ctx)  override;
        void OnResume(const StateContext& ctx)  override;
        void OnExit(const StateContext& ctx)   override;

        bool OnEvent(const StateContext& ctx, Event& e)  override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void            Render(const StateContext& ctx)           override;

        std::string Name() const override { return "Gameplay"; }

    private:
        void OnKeyPressed(KeyPressedEvent& ke);
    private:
        DrawableFactory m_drawFactory;
        StateFactory                   m_pauseFactory;
        std::shared_ptr<IGameState>    m_pendingPush;
        std::unique_ptr<IDrawable> m_drawable;

        ObjectTransform            m_transform;

        // Input speeds (could come from config)
        float m_moveSpeed = 0.6f;
        float m_scaleSpeed = 0.5f;
        float m_rotateSpeed = 1.8f;

        // Stats logging
        float m_totalTime = 0.f;
        int   m_frameCount = 0;
        float m_logTimer = 0.f;
        static constexpr float kLogInterval = 1.f;
    };

}
