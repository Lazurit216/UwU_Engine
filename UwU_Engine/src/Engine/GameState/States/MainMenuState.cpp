#include "uwupch.h"
#include "MainMenuState.h"

namespace UwU_Engine
{

    MainMenuState::MainMenuState(StateFactory gameplayFactory)
        : m_gameplayFactory(std::move(gameplayFactory))
    {
    }

    void MainMenuState::OnEnter(const StateContext& ctx)
    {
        m_timeInMenu = 0.0f;
        UWU_ENGINE_INFO("[MainMenu] Entered");

        /*if (ctx.renderer)
            ctx.renderer->SetClearColor(0.08f, 0.08f, 0.12f); */
    }

    void MainMenuState::OnResume(const StateContext& ctx)
    {
        m_timeInMenu = 0.0f;
        UWU_ENGINE_INFO("[MainMenu] Resumed (returned from sub-state)");
        /*if (ctx.renderer)
            ctx.renderer->SetClearColor(0.08f, 0.08f, 0.12f);*/
    }

    StateTransition MainMenuState::Update(const StateContext& ctx, float dt)
    {
        m_timeInMenu += dt;

        // Prototype: auto-launch gameplay after kAutoPlayAfterSeconds.
        // In a real game, replace this with an ImGui "Play" button check.
        if (m_timeInMenu >= kAutoPlayAfterSeconds)
        {
            UWU_ENGINE_INFO("[MainMenu] Auto-starting gameplay (replace with UI input)");
            return StateTransition::Replace(m_gameplayFactory());
        }

        return StateTransition::None();
    }

    void MainMenuState::Render(const StateContext& ctx)
    {
        // TODO: ImGui::Begin("Main Menu") … ImGui::Button("Play") … ImGui::End()
    }

} 
