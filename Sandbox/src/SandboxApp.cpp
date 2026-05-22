#include "uwupch.h"
#include "UwU.h"
#include "Engine/Config.h"
#include "Engine/Renderer/DirectX12/DX12Renderer.h"
#include "Engine/GameState/States/LoadingState.h"
#include "Engine/GameState/States/GameplayState.h"
#include "Engine/GameState/States/GamePauseState.h"
#include "Engine/GameState/States/EditorState.h"

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
        RegisterContext(std::move(c0));

        // State chain (state manager runs against primary renderer)
        auto makePause = []() { return std::make_shared<GamePauseState>(); };
        auto makeGameplay = [makePause, shaderPath, scenePath]() -> std::shared_ptr<GameplayState>
            {
                return std::make_shared<GameplayState>(makePause, shaderPath, scenePath);
            };
        auto makeEditor = [makeGameplay, scenePath]() -> std::shared_ptr<IGameState>
            {
                return std::make_shared<EditorState>(makeGameplay(), scenePath);
            };

        LoadingState::ScenePreloadDesc preloadDesc;
        preloadDesc.scenePath = scenePath;
        preloadDesc.fallbackShaderPath = shaderPath;
        InitStateManager(std::make_shared<LoadingState>(makeEditor, std::move(preloadDesc), 0.2f));

        UWU_ENGINE_INFO("[Sandbox] OnInit complete - 1 window");
        return true;
    }

private:
    Config m_cfg;
};

UwU_Engine::Application* UwU_Engine::CreateApplication()
{
    return new Sandbox();
}
