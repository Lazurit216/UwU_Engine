#include "uwupch.h"
#include "GamePauseState.h"

namespace UwU_Engine
{
    void GamePauseState::OnEnter(const StateContext& ctx)
    {
        m_shouldPop = false;
        UWU_ENGINE_INFO("[GamePause] Entered");
    }

    void GamePauseState::OnExit(const StateContext& ctx)
    {
        UWU_ENGINE_INFO("[GamePause] Exited");
    }

    void GamePauseState::OnPause(const StateContext& ctx)
    {
        // Another state pushed on top of pause — unlikely but handle cleanly
        UWU_ENGINE_INFO("[GamePause] Paused");
    }

    bool GamePauseState::OnEvent(const StateContext& ctx, Event& e)
    {
        EventDispatcher d(e);
        d.Dispatch<KeyPressedEvent>(std::bind(&GamePauseState::OnKeyPressed, this, std::placeholders::_1), EventType::KeyPressed);

        return e.Handled;
    }
    void GamePauseState::OnKeyPressed(KeyPressedEvent& ke)
    {
        if (ke.GetKeyCode() == VK_ESCAPE)
        {
            UWU_ENGINE_INFO("[GamePause] Escape — resuming gameplay");
            m_shouldPop = true;
            ke.Handled = true;  // prevent Gameplay below from seeing this
        }
    }

    StateTransition GamePauseState::Update(const StateContext& ctx, float dt)
    {
        if (m_shouldPop)
            return StateTransition::Pop();

        return StateTransition::None();
    }

    void GamePauseState::Render(const StateContext& ctx)
    {
        // TODO: draw pause overlay with ImGui
    }
}