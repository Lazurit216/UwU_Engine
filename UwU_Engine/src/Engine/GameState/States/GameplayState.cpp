#include "uwupch.h"
#include "GameplayState.h"

namespace UwU_Engine
{
    namespace
    {
        std::string MakeSceneSavePath(const std::string& scenePath)
        {
            if (scenePath.empty())
                return "Assets\\Scenes\\scene_test_saved.json";

            std::filesystem::path path(scenePath);
            const std::string stem = path.stem().string();
            const std::string extension = path.extension().string();
            path.replace_filename(stem + "_saved" + extension);
            return path.string();
        }
    }

    GameplayState::GameplayState(StateFactory pauseFactory, std::wstring shaderPath, std::string scenePath)
        : m_pauseFactory(std::move(pauseFactory))
        , m_shaderPath(std::move(shaderPath))
        , m_scenePath(std::move(scenePath))
        , m_sceneSavePath(MakeSceneSavePath(m_scenePath))
    {
    }

    void GameplayState::OnEnter(const StateContext& ctx)
    {
        m_totalTime = m_logTimer = 0.f;
        m_frameCount = 0;
        m_pendingPush.reset();

        m_renderSystem.Shutdown(m_world);
        if (m_scenePath.empty() || !m_sceneSerializer.Load(m_world, m_scenePath, m_shaderPath))
        {
            UWU_ENGINE_WARN("[Gameplay] Scene file unavailable - using built-in ECS demo scene");
            BuildDemoScene();
        }
        else
        {
            BindDemoSceneEntities();
            UWU_ENGINE_INFO("[Gameplay] Scene loaded from '{}'", m_scenePath);
        }

        if (!ctx.renderer)
            UWU_ENGINE_ERROR("[Gameplay] OnEnter: ctx.renderer is null");

        UWU_ENGINE_INFO("[Gameplay] ECS scene created with {} entities", m_world.EntityCount());
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
        m_renderSystem.Shutdown(m_world);
        m_world.Clear();
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
            UWU_ENGINE_INFO("[Gameplay] Escape - pushing pause");
            m_pendingPush = m_pauseFactory();
            ke.Handled = true;
            return;
        }

        if (ke.GetKeyCode() == VK_F5)
        {
            m_sceneSerializer.Save(m_world, m_sceneSavePath);
            ke.Handled = true;
            return;
        }

        if (ke.GetKeyCode() == VK_F9)
        {
            m_renderSystem.Shutdown(m_world);
            if (!m_sceneSerializer.Load(m_world, m_sceneSavePath, m_shaderPath))
                m_sceneSerializer.Load(m_world, m_scenePath, m_shaderPath);
            BindDemoSceneEntities();
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

        UpdateDemoScene(ctx, dt);
        return StateTransition::None();
    }

    void GameplayState::Render(const StateContext& ctx)
    {
        static int frameCount = 0;
        if (frameCount++ < 3)
            UWU_ENGINE_INFO("[Gameplay] Render frame {} - ECS entities={}",
                frameCount, m_world.EntityCount());

        if (!ctx.renderer)
        {
            static bool warned = false;
            if (!warned)
            {
                UWU_ENGINE_WARN("[Gameplay] Render: renderer is null");
                warned = true;
            }
            return;
        }

        m_renderSystem.Render(m_world, ctx.renderer);
    }

    void GameplayState::BuildDemoScene()
    {
        m_renderSystem.Shutdown(m_world);
        m_world.Clear();
        m_controlledEntity = kInvalidEntity;
        m_controlledChildEntity = kInvalidEntity;
        m_spinnerEntity = kInvalidEntity;
        m_cameraEntity = kInvalidEntity;

        m_controlledEntity = CreateTriangleEntity(
            "Player triangle",
            TransformComponent{ -0.85f, -0.30f, 0.0f, 0.0f, 0.42f, 0.42f, 1.0f },
            Color4{ 0.95f, 0.15f, 0.18f, 1.0f });

        m_controlledChildEntity = CreateTriangleEntity(
            "Player child triangle",
            TransformComponent{ 1.05f, 0.0f, 0.0f, 0.0f, 0.45f, 0.45f, 1.0f },
            Color4{ 1.0f, 0.95f, 0.25f, 1.0f });

        m_spinnerEntity = CreateTriangleEntity(
            "Auto spinner",
            TransformComponent{ -0.20f, 0.28f, 0.0f, 0.0f, 0.34f, 0.34f, 1.0f },
            Color4{ 0.15f, 0.90f, 0.30f, 1.0f });

        CreateTriangleEntity(
            "Static blue triangle",
            TransformComponent{ 0.15f, -0.42f, 0.0f, 0.35f, 0.30f, 0.30f, 1.0f },
            Color4{ 0.20f, 0.45f, 1.0f, 1.0f });

        auto& parentHierarchy = m_world.AddComponent<HierarchyComponent>(m_controlledEntity);
        auto& childHierarchy = m_world.AddComponent<HierarchyComponent>(m_controlledChildEntity);
        childHierarchy.parent = m_controlledEntity;
        parentHierarchy.children.push_back(m_controlledChildEntity);

        m_cameraEntity = CreateCameraEntity();

        UWU_ENGINE_INFO("[Gameplay] ECS demo scene initialized");
    }

    void GameplayState::BindDemoSceneEntities()
    {
        m_controlledEntity = FindEntityByTag("Player triangle");
        m_controlledChildEntity = FindEntityByTag("Player child triangle");
        m_spinnerEntity = FindEntityByTag("Auto spinner");
        m_cameraEntity = FindEntityByTag("Main camera");
    }

    EntityId GameplayState::FindEntityByTag(const std::string& name)
    {
        EntityId result = kInvalidEntity;
        m_world.ForEach<TagComponent>(
            [&result, &name](EntityId entity, TagComponent& tag)
            {
                if (result == kInvalidEntity && tag.name == name)
                    result = entity;
            });
        return result;
    }

    EntityId GameplayState::CreateTriangleEntity(const std::string& name,
        const TransformComponent& transform, const Color4& color)
    {
        EntityId entity = m_world.CreateEntity();
        m_world.AddComponent<TagComponent>(entity, TagComponent{ name });
        m_world.AddComponent<TransformComponent>(entity, transform);

        MaterialDesc material;
        material.baseColor = color;
        material.shaderPath = m_shaderPath;

        m_world.AddComponent<MeshRendererComponent>(
            entity,
            MeshRendererComponent{ MeshFactory::CreateTriangle(1.0f, color), std::move(material) });

        UWU_ENGINE_INFO("[Gameplay] Entity {} created ({})", entity, name);
        return entity;
    }

    EntityId GameplayState::CreateCameraEntity()
    {
        EntityId entity = m_world.CreateEntity();
        m_world.AddComponent<TagComponent>(entity, TagComponent{ "Main camera" });
        m_world.AddComponent<TransformComponent>(
            entity,
            TransformComponent{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f });

        CameraComponent camera;
        camera.primary = true;
        camera.zoom = 1.0f;
        camera.viewHalfWidth = 1.8f;
        camera.viewHalfHeight = 1.0f;
        camera.moveSpeed = 0.8f;
        camera.zoomSpeed = 1.0f;
        m_world.AddComponent<CameraComponent>(entity, camera);

        UWU_ENGINE_INFO("[Gameplay] Entity {} created (Main camera)", entity);
        return entity;
    }

    void GameplayState::UpdateDemoScene(const StateContext& ctx, float dt)
    {
        m_cameraSystem.Update(m_world, ctx.input, dt);

        if (auto* spinner = m_world.GetComponent<TransformComponent>(m_spinnerEntity))
        {
            spinner->rotationZ += 1.4f * dt;
            spinner->y = 0.28f + std::sin(m_totalTime * 1.6f) * 0.08f;
        }

        if (auto* child = m_world.GetComponent<TransformComponent>(m_controlledChildEntity))
            child->rotationZ -= 1.7f * dt;

        if (!ctx.input)
            return;

        auto* controlled = m_world.GetComponent<TransformComponent>(m_controlledEntity);
        if (!controlled)
            return;

        auto& input = *ctx.input;

        if (input.IsKeyDown(VK_LEFT))  controlled->x -= m_moveSpeed * dt;
        if (input.IsKeyDown(VK_RIGHT)) controlled->x += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_UP))    controlled->y += m_moveSpeed * dt;
        if (input.IsKeyDown(VK_DOWN))  controlled->y -= m_moveSpeed * dt;

        controlled->x = std::clamp(controlled->x, -1.8f, 1.8f);
        controlled->y = std::clamp(controlled->y, -1.8f, 1.8f);

        const float dx = input.GetMouseDeltaX();
        const float dy = input.GetMouseDeltaY();

        if (input.IsMouseDown(MouseButton::Right))
            controlled->rotationZ -= dx * m_rotateSpeed * 0.005f;

        if (input.IsMouseDown(MouseButton::Left))
        {
            float driver = (std::abs(dy) >= std::abs(dx)) ? -dy : dx;
            float scale = std::clamp(
                controlled->scaleX + driver * m_scaleSpeed * 0.005f,
                0.1f, 5.0f);

            controlled->scaleX = scale;
            controlled->scaleY = scale;
            controlled->scaleZ = scale;
        }
    }
}
