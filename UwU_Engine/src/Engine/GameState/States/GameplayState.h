#pragma once
// Prototype gameplay state.
// Logs deltatime each frame (satisfies the "deltatime logging" requirement).
// In a real project this state would own the scene, camera, entities, etc.

#include "Engine/GameState/IGameState.h"

namespace UwU_Engine
{

    class UWU_API GameplayState : public IGameState
    {
    public:
        using StateFactory = std::function<std::shared_ptr<IGameState>()>;

        explicit GameplayState(StateFactory pauseFactory = nullptr);

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
        StateFactory                   m_pauseFactory;
        std::shared_ptr<IGameState>    m_pendingPush;

        float m_totalTime = 0.0f;
        int   m_frameCount = 0;

        static constexpr float kLogIntervalSeconds = 1.0f;
        float m_logTimer = 0.0f;
    };

}