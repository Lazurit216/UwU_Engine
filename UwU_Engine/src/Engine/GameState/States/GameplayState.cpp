#include "uwupch.h"
#include "GameplayState.h"
//#include "Engine/Renderer/IRenderer.h"

namespace UwU_Engine
{
    GameplayState::GameplayState(StateFactory pauseFactory, DrawableFactory drawFactory)
        : m_pauseFactory(std::move(pauseFactory))
        , m_drawFactory(std::move(drawFactory))
    {
    }

    void GameplayState::OnEnter(const StateContext& ctx)
    {
        m_totalTime = m_logTimer = 0.f;
        m_frameCount = 0;
        m_transform = {};

        // Factory is provided by Sandbox — GameplayState never sees DX12Triangle.
        if (ctx.renderer && m_drawFactory)
        {
            m_drawable = m_drawFactory(ctx.renderer);
            if (!m_drawable)
                UWU_ENGINE_WARN("Gameplay", "DrawFactory returned null — no geometry");
        }
        UWU_ENGINE_INFO("Gameplay", "Entered");
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
        UWU_ENGINE_INFO("[Gameplay] Exited after {:.2f}s, {} frames", m_totalTime, m_frameCount);
        if (m_drawable) { m_drawable->Shutdown(); m_drawable.reset(); }
    }

    bool GameplayState::OnEvent(const StateContext& ctx, Event& e)
    {
        EventDispatcher d(e);
        d.Dispatch<KeyPressedEvent>(
            std::bind(&GameplayState::OnKeyPressed, this, std::placeholders::_1),
            EventType::KeyPressed);
        return e.Handled;
    }

    void GameplayState::OnKeyPressed(KeyPressedEvent& ke)
    {
        if (ke.GetKeyCode() == VK_ESCAPE && m_pauseFactory)
        {
            UWU_ENGINE_INFO("[Gameplay] Escape — pushing pause");
            m_pendingPush = m_pauseFactory();
            ke.Handled = true;
        }
    }

    StateTransition GameplayState::Update(const StateContext& ctx, float dt)
    {
        if (m_pendingPush)
        {
            auto s = std::move(m_pendingPush);
            return StateTransition::Push(s);
        }

        m_totalTime += dt;
        m_logTimer += dt;
        m_frameCount += 1;

        if (m_logTimer >= kLogInterval)
        {
            float avgDt = m_logTimer / static_cast<float>(m_frameCount);
            UWU_ENGINE_INFO("[Gameplay] avg dt={:.4f}ms  fps={:.1f}  total={:.1f}s",
                avgDt * 1000.f, avgDt > 0.f ? 1.f / avgDt : 0.f, m_totalTime);
            m_logTimer = 0.f;
            m_frameCount = 0;
        }

        // ── Transform update via InputManager ─────────────────────────────────
        if (!ctx.input || !m_drawable) return StateTransition::None();

        auto& input = *ctx.input;

        // Arrow keys: smooth movement
        if (input.IsKeyDown(VK_LEFT))  m_transform.x -= m_moveSpeed * dt;
        if (input.IsKeyDown(VK_RIGHT)) m_transform.x += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_UP))    m_transform.y += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_DOWN))  m_transform.y -= m_moveSpeed * dt;
        m_transform.x = std::clamp(m_transform.x, -1.8f, 1.8f);
        m_transform.y = std::clamp(m_transform.y, -1.8f, 1.8f);

        const float dx = input.GetMouseDeltaX();
        const float dy = input.GetMouseDeltaY();

        // RMB: horizontal delta -> rotation.  dx > 0 (right) = clockwise = negative angle.
        if (input.IsMouseDown(MouseButton::Right))
            m_transform.rotation -= dx * m_rotateSpeed * 0.005f;

        // LMB: Y-axis dominates (dy > 0 = screen down = shrink), fall back to X.
        if (input.IsMouseDown(MouseButton::Left))
        {
            float driver = (std::abs(dy) >= std::abs(dx)) ? -dy : dx;
            m_transform.scale = std::clamp(
                m_transform.scale + driver * m_scaleSpeed * 0.005f, 0.1f, 5.0f);
        }

        m_drawable->SetTransform(m_transform);
        return StateTransition::None();
    }

    void GameplayState::Render(const StateContext& ctx)
    {
        // IDrawable::Draw() is self-contained — no DX12 types needed here.
        if (m_drawable && m_drawable->IsReady())
            m_drawable->Draw();
    }

}