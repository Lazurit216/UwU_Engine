#pragma once
#include "Engine/GameState/IGameState.h"

namespace UwU_Engine
{

    class UWU_API GamePauseState : public IGameState
    {
    public:
        void OnEnter(const StateContext& ctx)  override;
        void OnPause(const StateContext& ctx)  override;
        void OnExit(const StateContext& ctx)   override;

        bool OnEvent(const StateContext& ctx, Event& e)  override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void            Render(const StateContext& ctx)           override;

        std::string Name() const override { return "GamePause"; }

    private:
        void OnKeyPressed(KeyPressedEvent& ke);
    private:
        bool m_shouldPop = false;
    };

}