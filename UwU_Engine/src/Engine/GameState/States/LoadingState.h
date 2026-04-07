#pragma once
// Simulates async asset loading. After a configurable duration it
// automatically transitions to whatever state you supply as "next".
// In a real engine, replace the timer with actual async IO futures.

#include "Engine/GameState/IGameState.h"

namespace UwU_Engine
{

    class UWU_API LoadingState : public IGameState
    {
    public:
        // nextFactory — called when loading finishes to create the next state.
        // Using a factory (not the state itself) avoids constructing heavy
        // game objects until they are actually needed.
        using StateFactory = std::function<std::shared_ptr<IGameState>()>;

        explicit LoadingState(StateFactory nextFactory,
            float        simulatedLoadSeconds = 2.0f);

        void            OnEnter(const StateContext& ctx)   override;
        StateTransition Update(const StateContext& ctx, float dt) override;
        void            Render(const StateContext& ctx)    override;
        std::string     Name()  const                      override { return "Loading"; }

    private:
        StateFactory m_nextFactory;
        float        m_loadDuration;
        float        m_elapsed = 0.0f;
    };

}
