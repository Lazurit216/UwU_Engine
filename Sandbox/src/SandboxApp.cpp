#include "uwupch.h"
#include "UwU.h"
#include "Engine/Config.h"
#include "Engine/Renderer/DirectX12/DX12Renderer.h"
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
        const std::string scenePath = configBase + "Assets\\Scenes\\scene_test.json";
        m_cfg.Load(configPath);

        Window* win0 = m_windowManager.Create(
            { m_cfg.GetInt("windows.primary.width",  1280),
              m_cfg.GetInt("windows.primary.height", 720),
              m_cfg.GetString("windows.primary.title", "UwU Engine - Primary") });
        if (!win0) return false;

        auto r0 = std::make_unique<DX12Renderer>();
        if (!r0->Init(win0->GetNativeHandle(), win0->GetWidth(), win0->GetHeight()))
            return false;

        auto bg0 = m_cfg.GetColor3("windows.primary.bgColor", { 0.0f, 0.0f, 0.0f });
        r0->SetClearColor(bg0[0], bg0[1], bg0[2]);

        WindowContext c0;
        c0.window = win0;
        c0.renderer = std::move(r0);
        RegisterContext(std::move(c0));         // idx 0 - primary

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

        auto bg1 = m_cfg.GetColor3("windows.secondary.bgColor", { 0.0f, 0.0f, 0.0f });
        r1->SetClearColor(bg1[0], bg1[1], bg1[2]);

        WindowContext c1;
        c1.window = win1;
        c1.renderer = std::move(r1);
        RegisterContext(std::move(c1));          // idx 1 - secondary

        m_secondaryDesc.material.shaderPath = shaderPath;
        m_secondaryDesc.material.baseColor = { 1.f, 0.85f, 0.15f, 1.f };
        m_secondaryDesc.mesh = MeshFactory::CreateTriangle(1.2f, m_secondaryDesc.material.baseColor);

        // State chain (state manager runs against primary renderer)
        auto makePause = []() { return std::make_shared<GamePauseState>(); };
        auto makeGameplay = [makePause, shaderPath, scenePath]() -> std::shared_ptr<IGameState>
            {
                return std::make_shared<GameplayState>(makePause, shaderPath, scenePath);
            };
        auto makeMenu = [makeGameplay]() { return std::make_shared<MainMenuState>(makeGameplay); };

        LoadingState::ScenePreloadDesc preloadDesc;
        preloadDesc.scenePath = scenePath;
        preloadDesc.fallbackShaderPath = shaderPath;
        InitStateManager(std::make_shared<LoadingState>(makeMenu, std::move(preloadDesc), 0.2f));

        UWU_ENGINE_INFO("[Sandbox] OnInit complete - 2 windows, shared DX12 device");
        return true;
    }

    void OnFixedUpdate(float fixedDt) override
    {
        m_secondaryRotation += 1.2f * fixedDt;   // steady ~1.2 rad/s regardless of FPS
        if (!m_secondaryDrawable)
            return;

        ObjectTransform t = m_secondaryDrawable->GetTransform();
        t.rotation = m_secondaryRotation;
        m_secondaryDrawable->SetTransform(t);
    }

    void OnContextRender(int idx, IRenderer* renderer) override
    {
        if (idx != 1 || !renderer)
            return;

        if (!m_secondaryDrawable)
        {
            m_secondaryDrawable = renderer->CreateDrawable(m_secondaryDesc);
            if (m_secondaryDrawable)
            {
                ObjectTransform t;
                t.scale = 1.0f;
                t.rotation = m_secondaryRotation;
                m_secondaryDrawable->SetTransform(t);
            }
            else
            {
                UWU_WARN("Sandbox", "Secondary triangle Init failed - secondary window will be empty");
                return;
            }
        }

        if (m_secondaryDrawable->IsReady())
            m_secondaryDrawable->Draw();
    }

private:
    Config m_cfg;

    DrawableDesc m_secondaryDesc;
    std::unique_ptr<IDrawable> m_secondaryDrawable;
    float m_secondaryRotation = 0.0f;
};

UwU_Engine::Application* UwU_Engine::CreateApplication()
{
    return new Sandbox();
}
