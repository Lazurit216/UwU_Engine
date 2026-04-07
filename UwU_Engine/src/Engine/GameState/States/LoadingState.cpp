#include "uwupch.h"
#include "LoadingState.h"
//#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{

    LoadingState::LoadingState(StateFactory nextFactory, float loadSeconds)
        : m_nextFactory(std::move(nextFactory))
        , m_loadDuration(loadSeconds)
    {
    }

    void LoadingState::OnEnter(const StateContext& ctx)
    {
        m_elapsed = 0.0f;
        UWU_ENGINE_INFO("[LoadingState] Simulating {}s asset load", m_loadDuration);

        // Give the screen a dark teal clear colour while loading
        /*if (ctx.renderer)
            ctx.renderer->SetClearColor(0.05f, 0.12f, 0.15f);*/
    }

    StateTransition LoadingState::Update(const StateContext& ctx, float dt)
    {
        m_elapsed += dt;

        float progress = m_elapsed / m_loadDuration;
        if (progress > 1.0f) progress = 1.0f;

        // Periodic progress log (every 25%)
        static float lastLog = 0.0f;
        if (progress - lastLog >= 0.25f)
        {
            UWU_ENGINE_INFO("[LoadingState] Progress: {:.0f}%", progress * 100.0f);
            lastLog = progress;
        }

        if (m_elapsed >= m_loadDuration)
        {
            lastLog = 0.0f;
            UWU_ENGINE_INFO("[LoadingState] Loading complete — transitioning");
            return StateTransition::Replace(m_nextFactory());
        }

        return StateTransition::None();
    }

    void LoadingState::Render(const StateContext& ctx)
    {
        // In a real engine: draw a progress bar / spinner using ImGui here.
        // The renderer's BeginFrame/EndFrame are handled by Application.
    }

} 
