#include <UwU.h>
#include "Engine/Config.h"
#include "Engine/Input/InputManager.h"
#include "Engine/Renderer/DirectX12/DX12Renderer.h"
#include "Engine/Assets/DX12Apps/DX12Triangle.h"
#include "Engine/GameState/States/LoadingState.h"
#include "Engine/GameState/States/MainMenuState.h"
#include "Engine/GameState/States/GameplayState.h"
#include "Engine/GameState/States/GamePauseState.h"

using namespace UwU_Engine;

class Sandbox : public Application
{
public:
    Sandbox() = default;
    ~Sandbox() = default;

protected:
    bool OnInit() override
    {
        std::wstring shaderBase;
        std::string configBase;
        if (IsDebuggerPresent())
        {
            UWU_INFO("Debugger detected - using source-tree path");
            shaderBase = L"..\\..\\Sandbox\\src\\";
            configBase = "..\\..\\Sandbox\\src\\";
        }
        const std::wstring shaderPath = shaderBase + L"Assets\\Shaders\\Color.hlsl";
        const std::string configPath= configBase+ "engine_config.json";
        m_cfg.Load(configPath);

        m_moveSpeed = m_cfg.GetFloat("input.moveSpeed", 0.6f);
        m_scaleSpeed = m_cfg.GetFloat("input.scaleSpeed", 0.5f);
        m_rotateSpeed = m_cfg.GetFloat("input.rotateSpeed", 1.8f);

        Window* win0 = m_windowManager.Create(
            { m_cfg.GetInt("windows.primary.width",  1280),
              m_cfg.GetInt("windows.primary.height", 720),
              m_cfg.GetString("windows.primary.title", "UwU Engine - Primary") });
        if (!win0) return false;

        auto r0 = std::make_unique<DX12Renderer>();
        if (!r0->Init(win0->GetNativeHandle(), win0->GetWidth(), win0->GetHeight()))
            return false;

        auto bg0 = m_cfg.GetColor3("windows.primary.bgColor", { 0.05f, 0.05f, 0.15f });
        r0->SetClearColor(bg0[0], bg0[1], bg0[2]);

        DX12Renderer* r0Raw = r0.get();           // save raw ptr before move

        TriangleDesc d0;
        d0.ShaderPath = shaderPath;
        d0.Vertices = { {
            {{ 0.0f,  0.5f, 0.0f }, { 1.f, 0.f, 0.f }},  // top   — red
            {{ 0.5f, -0.5f, 0.0f }, { 0.f, 1.f, 0.f }},  // right — green
            {{-0.5f, -0.5f, 0.0f }, { 0.f, 0.f, 1.f }},  // left  — blue
        } };
        auto t0 = std::make_unique<DX12Triangle>();
        if (!t0->Init(r0Raw, d0)) { UWU_WARN("[Sandbox] triangle0 failed"); t0.reset(); }

        WindowContext c0;
        c0.window = win0;
        c0.renderer = std::move(r0);
        c0.triangle = std::move(t0);
        RegisterContext(std::move(c0));         // idx 0 — primary

        //Secondary window
        auto* sharedDX12 = static_cast<DX12Renderer*>(GetContext(0).renderer.get());

        Window* win1 = m_windowManager.Create(
            { m_cfg.GetInt("windows.secondary.width",  800),
              m_cfg.GetInt("windows.secondary.height", 600),
              m_cfg.GetString("windows.secondary.title", "UwU Engine - Secondary") });
        if (!win1) return false;

        auto r1 = std::make_unique<DX12Renderer>();
        if (!r1->InitShared(win1->GetNativeHandle(), win1->GetWidth(), win1->GetHeight(),
            sharedDX12->GetDevice(), sharedDX12->GetFactory()))
            return false;

        auto bg1 = m_cfg.GetColor3("windows.secondary.bgColor", { 0.15f, 0.07f, 0.03f });
        r1->SetClearColor(bg1[0], bg1[1], bg1[2]);

        TriangleDesc d1;
        d1.ShaderPath = shaderPath;
        d1.Vertices = { {
            {{ 0.0f,  0.6f, 0.0f }, { 1.f, 1.f, 0.f }},  // top   — yellow
            {{ 0.6f, -0.4f, 0.0f }, { 1.f, 0.5f, 0.f }}, // right — orange
            {{-0.6f, -0.4f, 0.0f }, { 1.f, 1.f, 1.f }},  // left  — white
        } };
        auto t1 = std::make_unique<DX12Triangle>();
        if (!t1->Init(r1.get(), d1)) { UWU_ENGINE_WARN("[Sandbox] triangle1 failed"); t1.reset(); }

        WindowContext c1;
        c1.window = win1;
        c1.renderer = std::move(r1);
        c1.triangle = std::move(t1);
        RegisterContext(std::move(c1));         // idx 1 — secondary

        // ── State chain (state manager runs against primary renderer) ─────────
        auto makePause = []() { return std::make_shared<GamePauseState>(); };
        auto makeGameplay = [makePause]() { return std::make_shared<GameplayState>(makePause); };
        auto makeMenu = [makeGameplay]() { return std::make_shared<MainMenuState>(makeGameplay); };

        InitStateManager(std::make_shared<LoadingState>(makeMenu, 2.0f));

        UWU_ENGINE_INFO("[Sandbox] OnInit complete - 2 windows, shared DX12 device");
        return true;
    }

    void OnUpdate(float dt) override
    {
        UpdateTriangle(GetContext(0), dt);
        UpdateTriangle(GetContext(1), dt);
    }

private:
    void UpdateTriangle(WindowContext& ctx, float dt)
    {
        auto* tri = ctx.triangle.get();
        if (!tri) return;

        auto& input = ctx.input;
        ObjectTransform t = tri->GetTransform();

        if (input.IsKeyDown(VK_LEFT))  t.x -= m_moveSpeed * dt;
        if (input.IsKeyDown(VK_RIGHT)) t.x += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_UP))    t.y += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_DOWN))  t.y -= m_moveSpeed * dt;
        t.x = std::clamp(t.x, -1.8f, 1.8f);
        t.y = std::clamp(t.y, -1.8f, 1.8f);

        const float dx = input.GetMouseDeltaX();
        const float dy = input.GetMouseDeltaY();

        if (input.IsMouseDown(MouseButton::Right))
            t.rotation -= dx * m_rotateSpeed * 0.005f;

        if (input.IsMouseDown(MouseButton::Left))
        {
            float scaleDriver = (std::abs(dy) >= std::abs(dx)) ? -dy : dx;
            t.scale = std::clamp(t.scale + scaleDriver * m_scaleSpeed * 0.005f,
                0.1f, 5.0f);
        }

        tri->SetTransform(t);
    }

    Config m_cfg;
    float  m_moveSpeed = 0.6f;
    float  m_scaleSpeed = 0.5f;
    float  m_rotateSpeed = 1.8f;
};

UwU_Engine::Application* UwU_Engine::CreateApplication()
{
    return new Sandbox();
}
