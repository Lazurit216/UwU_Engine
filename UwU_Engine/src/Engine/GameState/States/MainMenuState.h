#pragma once
#include "Engine/GameState/IGameState.h"

namespace UwU_Engine
{

    class UWU_API MainMenuState : public IGameState
    {
    public:
        using StateFactory = std::function<std::shared_ptr<IGameState>()>;

        // gameplayFactory — called when "Play" is chosen
        explicit MainMenuState(StateFactory gameplayFactory);

        void            OnEnter(const StateContext& ctx)   override;
        void            OnResume(const StateContext& ctx)  override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void            Render(const StateContext& ctx)    override;
        std::string     Name()  const                      override { return "MainMenu"; }

    private:
        StateFactory m_gameplayFactory;
        float        m_timeInMenu = 0.0f;

        // Simple auto-play trigger for the prototype (simulates pressing Play).
        // Replace with ImGui button input once ImGui is wired to the renderer.
        static constexpr float kAutoPlayAfterSeconds = 3.0f;
    };

} 
