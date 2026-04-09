#include "uwupch.h"
#include "GameplayState.h"
//#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{
    GameplayState::GameplayState(StateFactory pauseFactory)
        : m_pauseFactory(std::move(pauseFactory))
    {}

    void GameplayState::OnEnter(const StateContext& ctx)
    {
        m_totalTime = 0.0f;
        m_frameCount = 0;
        m_logTimer = 0.0f;
        UWU_ENGINE_INFO("[Gameplay] Entered");

        //if (ctx.renderer)
        //    ctx.renderer->SetClearColor(0.1f, 0.18f, 0.1f); // dark green
    }

    void GameplayState::OnPause(const StateContext& ctx)
    {
        UWU_ENGINE_INFO("[Gameplay] Paused (pushed over by another state)");
    }

    void GameplayState::OnResume(const StateContext& ctx)
    {
        UWU_ENGINE_INFO("[Gameplay] Resumed");
        // Reset timer so the pause time doesn't inflate dt stats
        m_logTimer = 0.0f;
        m_frameCount = 0;
    }

    void GameplayState::OnExit(const StateContext& ctx)
    {
        UWU_ENGINE_INFO("[Gameplay] Exited after {:.2f}s, {} frames",
            m_totalTime, m_frameCount);
    }

    bool GameplayState::OnEvent(const StateContext& ctx, Event& e)
    {
        EventDispatcher d(e);
        d.Dispatch<KeyPressedEvent>(std::bind(&GameplayState::OnKeyPressed, this, std::placeholders::_1), EventType::KeyPressed);

        return e.Handled;
    }
    void GameplayState::OnKeyPressed(KeyPressedEvent& ke)
    {
        if (ke.GetKeyCode() == VK_ESCAPE && m_pauseFactory)
        {
            UWU_ENGINE_INFO("[Gameplay] Escape — pushing pause");
            m_pendingPush = m_pauseFactory();  // factory creates whatever pause state
            ke.Handled = true;
        }
    }

    StateTransition GameplayState::Update(const StateContext& ctx, float dt)
    {
        if (m_pendingPush)
        {
            auto state = std::move(m_pendingPush);
            return StateTransition::Push(state);
        }

        m_totalTime += dt;
        m_logTimer += dt;
        ++m_frameCount;

        if (m_logTimer >= kLogIntervalSeconds)
        {
            float avgDt = m_logTimer / static_cast<float>(m_frameCount);
            float fps = avgDt > 0.0f ? 1.0f / avgDt : 0.0f;

            UWU_ENGINE_INFO("[Gameplay] dt={:.4f}ms  fps={:.1f}  frame={}  total={:.1f}s",
                avgDt * 1000.0f, fps, m_frameCount, m_totalTime);

            m_logTimer = 0.0f;
            m_frameCount = 0;
        }


        // No transition — stay in gameplay until the window closes.
        return StateTransition::None();
    }

    void GameplayState::Render(const StateContext& ctx)
    {
        // TODO: issue draw calls through ctx.renderer once geometry is added.
        // For the prototype, the clear colour from OnEnter is sufficient output.
    }

}