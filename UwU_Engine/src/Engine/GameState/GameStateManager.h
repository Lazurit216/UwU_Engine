#pragma once
// Manages a stack of game states.
//
// Key design: transitions requested during Update() are DEFERRED -
// they are applied at the start of the next tick, not mid-frame.
// This prevents "use after free" crashes from a state deleting itself.
#include "IGameState.h"
#include "Engine/Core.h"

namespace UwU_Engine
{

    class UWU_API GameStateManager
    {
    public:
        GameStateManager() = default;
        ~GameStateManager();

        // Lifecycle

        // Push the very first state before the main loop starts.
        void Init(std::shared_ptr<IGameState> initialState, const StateContext& ctx);

        // Call once per frame — applies any pending transition, then calls
        // Update() on the top state.
        // Returns false if the stack is empty (signal to quit).
        bool Update(const StateContext& ctx, float dt);

        void OnEvent(const StateContext& ctx, Event& e);

        // Call once per frame — calls Render() on the active state.
        void Render(const StateContext& ctx);

        // True while at least one state is on the stack.
        bool HasStates() const { return !m_stack.empty(); }

        // Name of the currently active state (for debug UI).
        std::string ActiveStateName() const;

    private:
        void ApplyTransition(const StateContext& ctx, const StateTransition& t);

        std::vector<std::shared_ptr<IGameState>> m_stack;
        StateContext                             m_ctx;

        // Transition requested by the active state — applied next frame.
        std::optional<StateTransition>           m_pending;
    };

}
