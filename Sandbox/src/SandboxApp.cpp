#include <UwU.h>
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
        std::wstring sandboxSrcPath;
        if (IsDebuggerPresent()) 
        {
            UWU_INFO("Is debugger");
            sandboxSrcPath = L"..\\..\\Sandbox\\src\\";
        }

        // ── Primary window (1280x720, dark blue, RGB triangle) ────────────────
        Window* win0 = m_windowManager.Create({ 1280, 720, "UwU Engine - Primary" });
        if (!win0) return false;

        auto r0 = std::make_unique<DX12Renderer>();
        if (!r0->Init(win0->GetNativeHandle(), win0->GetWidth(), win0->GetHeight()))
            return false;
        r0->SetClearColor(0.05f, 0.05f, 0.15f);  // dark blue

        DX12Renderer* r0Raw = r0.get();           // save raw ptr before move

        TriangleDesc d0;
        d0.ShaderPath = sandboxSrcPath + L"Assets\\Shaders\\Color.hlsl";
        std::string shaderPathStr1(d0.ShaderPath.begin(), d0.ShaderPath.end());
        UWU_TRACE("Shaders path: {}", shaderPathStr1);
        d0.Vertices = { {
            {{ 0.0f,  0.5f, 0.0f }, { 1.f, 0.f, 0.f }},  // top   red
            {{ 0.5f, -0.5f, 0.0f }, { 0.f, 1.f, 0.f }},  // right green
            {{-0.5f, -0.5f, 0.0f }, { 0.f, 0.f, 1.f }},  // left  blue
        } };
        auto t0 = std::make_unique<DX12Triangle>();
        if (!t0->Init(r0Raw, d0)) { UWU_WARN("triangle0 failed"); t0.reset(); }

        WindowContext c0;
        c0.window = win0;
        c0.renderer = std::move(r0);
        c0.triangle = std::move(t0);
        RegisterContext(std::move(c0));         // idx 0 — primary

        // ── Secondary window (800x600, warm brown, yellow triangle) ───────────
        // Reuse the GPU device from renderer0 — no second device allocation.
        auto* sharedDX12 = static_cast<DX12Renderer*>(GetContext(0).renderer.get());

        Window* win1 = m_windowManager.Create({ 800, 600, "UwU Engine - Secondary" });
        if (!win1) return false;

        auto r1 = std::make_unique<DX12Renderer>();
        if (!r1->InitShared(win1->GetNativeHandle(), win1->GetWidth(), win1->GetHeight(),
            sharedDX12->GetDevice(), sharedDX12->GetFactory()))
            return false;
        r1->SetClearColor(0.15f, 0.07f, 0.03f);  // warm dark brown

        TriangleDesc d1;
        d1.ShaderPath = sandboxSrcPath+L"Assets\\Shaders\\Color.hlsl";
        std::string shaderPathStr2(d1.ShaderPath.begin(), d1.ShaderPath.end());
        UWU_TRACE("Shaders path: {}", shaderPathStr2);
        d1.Vertices = { {
            {{ 0.0f,  0.6f, 0.0f }, { 1.f, 1.f, 0.f }},  // top   yellow
            {{ 0.6f, -0.4f, 0.0f }, { 1.f, 0.5f, 0.f }}, // right orange
            {{-0.6f, -0.4f, 0.0f }, { 1.f, 1.f, 1.f }},  // left  white
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

        UWU_ENGINE_INFO("[Sandbox] OnInit complete — 2 windows, shared DX12 device");
        return true;
    }
};

UwU_Engine::Application* UwU_Engine::CreateApplication()
{
    return new Sandbox();
}
