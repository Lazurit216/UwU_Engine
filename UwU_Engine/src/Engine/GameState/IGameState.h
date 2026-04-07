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
#include "Engine//Events/Events.h"
#include "Engine//Events/WindowEvents.h"
#include "Engine//Events/KeyboardEvents.h"

namespace UwU_Engine
{

    class UWU_API IGameState;
    //class UWU_API IRenderer;
    //struct Timer;

    enum class TransitionType { None, Push, Pop, Replace };

    struct StateTransition
    {
        TransitionType type = TransitionType::None;
        std::shared_ptr<IGameState> nextState; // null when type == Pop / None

        static StateTransition None()
        {
            return { TransitionType::None, nullptr };
        }
        static StateTransition Pop()
        {
            return { TransitionType::Pop, nullptr };
        }
        static StateTransition Push(std::shared_ptr<IGameState> s)
        {
            return { TransitionType::Push, std::move(s) };
        }
        static StateTransition Replace(std::shared_ptr<IGameState> s)
        {
            return { TransitionType::Replace, std::move(s) };
        }
    };

    // State context 
    // Passed into every state method so states can access shared engine services
    // without coupling to a God-object Application singleton.

    struct StateContext
    {
        IRenderer* renderer = nullptr;
        // Add more services here as the engine grows:
        // AudioSystem* audio  = nullptr;
        // InputManager* input = nullptr;
    };

    // IGameState 

    class UWU_API IGameState
    {
    public:
        virtual ~IGameState() = default;

        // Called once by the manager when this state becomes the active top state.
        virtual void OnEnter(const StateContext& ctx) {}

        // Called once when this state is removed from the top of the stack.
        virtual void OnExit(const StateContext& ctx) {}

        // Called when a new state is pushed on top of this one.
        // Use to pause animations, sounds, etc.
        virtual void OnPause(const StateContext& ctx) {}

        // Called when the state above is popped off.
        virtual void OnResume(const StateContext& ctx) {}

        virtual bool OnEvent(const StateContext& ctx, Event& e) { return false; }

        // Update logic. Returns a StateTransition to request a state change.
        // dt — seconds since last frame.
        virtual StateTransition Update(const StateContext& ctx, float dt) = 0;

        // Issue draw calls (the renderer's BeginFrame/EndFrame are called
        // by Application, not the state; states only call renderer helpers).
        virtual void Render(const StateContext& ctx) = 0;

        // Human-readable name for debugging.
        virtual std::string Name() const = 0;
    };

} 
