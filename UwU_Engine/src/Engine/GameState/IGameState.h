#pragma once
// Base interface for all game states.
//
// State lifetime (called by GameStateManager):
//
//   OnEnter()  — called once when the state becomes active
//   OnExit()   — called once when the state is popped or replaced
//   OnPause()  — called when another state is PUSHED on top (stack)
//   OnResume() — called when the state on top is POPPED off (stack)
//
// Update / Render contract:
//   Update() returns a StateTransition.
//   The manager processes the transition AFTER the current frame finishes,
//   so states never see themselves replaced mid-update.
//
// Stack semantics:
//   Push    — add a new state on top; current state receives OnPause()
//   Pop     — remove top state; state below receives OnResume()
//   Replace — pop then push in one step (no OnPause / OnResume on the old)
//   None    — no change
#include "Engine/Core.h"
#include "Engine/Renderer/IRenderer.h"
#include "Engine/Events/Events.h"
#include "Engine/Events/WindowEvents.h"
#include "Engine/Events/KeyboardEvents.h"
#include "Engine/Input/InputManager.h"

namespace UwU_Engine
{
    //class InputManager;  
    class IGameState;

    enum class TransitionType { None, Push, Pop, Replace };

    struct StateTransition
    {
        TransitionType type = TransitionType::None;
        std::shared_ptr<IGameState> nextState;

        static StateTransition None() { return { TransitionType::None,    nullptr }; }
        static StateTransition Pop() { return { TransitionType::Pop,     nullptr }; }
        static StateTransition Push(std::shared_ptr<IGameState> s)
        {
            return { TransitionType::Push, std::move(s) };
        }
        static StateTransition Replace(std::shared_ptr<IGameState> s)
        {
            return { TransitionType::Replace, std::move(s) };
        }
    };

    // Context passed to every state method — avoids coupling to Application.
    struct StateContext
    {
        IRenderer* renderer = nullptr;
        InputManager* input = nullptr;   
    };

    class UWU_API IGameState
    {
    public:
        virtual ~IGameState() = default;

        virtual void OnEnter(const StateContext& ctx) {}
        virtual void OnExit(const StateContext& ctx) {}
        virtual void OnPause(const StateContext& ctx) {}
        virtual void OnResume(const StateContext& ctx) {}

        virtual bool            OnEvent(const StateContext& ctx, Event& e) { return false; }
        virtual StateTransition Update(const StateContext& ctx, float dt) = 0;
        virtual void            Render(const StateContext& ctx) = 0;
        virtual std::string     Name()  const = 0;
    };
}
