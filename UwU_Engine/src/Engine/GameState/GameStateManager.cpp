#include "uwupch.h"
#include "GameStateManager.h"

namespace UwU_Engine
{

    GameStateManager::~GameStateManager()
    {
        //m_stack.clear();
        // Drain the stack so every OnExit is called in order
        while (!m_stack.empty())
        {
            m_stack.back()->OnExit(m_ctx);
            m_stack.pop_back();
        }
    }

    void GameStateManager::Init(std::shared_ptr<IGameState> initialState,
        const StateContext& ctx)
    {
        m_ctx = ctx;
        m_stack.push_back(std::move(initialState));
        m_stack.back()->OnEnter(m_ctx);
        UWU_ENGINE_INFO("[States] Entered '{}'", m_stack.back()->Name());
    }

    bool GameStateManager::Update(const StateContext& ctx, float dt)
    {
        // 1. Apply any pending transition from the previous frame first.
        if (m_pending.has_value())
        {
            ApplyTransition(ctx, *m_pending);
            m_pending.reset();
        }

        if (m_stack.empty()) return false;

        // 2. Update the top state and collect any new transition request.
        StateTransition t = m_stack.back()->Update(ctx, dt);

        // Queue it — do NOT apply mid-frame.
        if (t.type != TransitionType::None)
            m_pending = t;

        return !m_stack.empty();
    }

    void GameStateManager::OnEvent(const StateContext& ctx, Event& e)
    {
        for (auto it = m_stack.rbegin(); it != m_stack.rend(); ++it)
        {
            (*it)->OnEvent(ctx, e);
            if (e.Handled) break;
        }
    }

    void GameStateManager::Render(const StateContext& ctx)
    {
        if (!m_stack.empty())
            m_stack.back()->Render(ctx);
    }

    std::string GameStateManager::ActiveStateName() const
    {
        return m_stack.empty() ? "<none>" : m_stack.back()->Name();
    }

    // Private

    void GameStateManager::ApplyTransition(const StateContext& ctx,
        const StateTransition& t)
    {
        switch (t.type)
        {
        case TransitionType::Pop:
            if (!m_stack.empty())
            {
                UWU_ENGINE_INFO("[States] Popping '{}'", m_stack.back()->Name());
                m_stack.back()->OnExit(ctx);
                m_stack.pop_back();

                if (!m_stack.empty())
                {
                    UWU_ENGINE_INFO("[States] Resuming '{}'", m_stack.back()->Name());
                    m_stack.back()->OnResume(ctx);
                }
            }
            break;

        case TransitionType::Push:
            if (t.nextState)
            {
                if (!m_stack.empty())
                {
                    UWU_ENGINE_INFO("[States] Pausing '{}'", m_stack.back()->Name());
                    m_stack.back()->OnPause(ctx);
                }
                m_stack.push_back(t.nextState);
                m_stack.back()->OnEnter(ctx);
                UWU_ENGINE_INFO("[States] Entered '{}'", m_stack.back()->Name());
            }
            break;

        case TransitionType::Replace:
            if (t.nextState)
            {
                if (!m_stack.empty())
                {
                    UWU_ENGINE_INFO("[States] Replacing '{}' with '{}'",
                        m_stack.back()->Name(), t.nextState->Name());
                    m_stack.back()->OnExit(ctx);
                    m_stack.pop_back();
                }
                m_stack.push_back(t.nextState);
                m_stack.back()->OnEnter(ctx);
                UWU_ENGINE_INFO("[States] Entered '{}'", m_stack.back()->Name());
            }
            break;

        case TransitionType::None:
        default:
            break;
        }
    }

} 
