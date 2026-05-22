#include "uwupch.h"
#include "GameplayState.h"
#include "Engine/Renderer/MeshGeometry.h"

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
        BindInputActions(ctx.input);
        SetupPhysicsEventLogging();
        m_physicsDebugRenderSystem.SetShaderPath(m_shaderPath);

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
        m_physicsDebugRenderSystem.Shutdown();
        m_physicsSystem.ClearEventListeners();
        m_world.Clear();
    }

    bool GameplayState::SaveSceneToFile(const std::string& scenePath)
    {
        return m_sceneSerializer.Save(m_world, scenePath);
    }

    bool GameplayState::LoadSceneFromFile(const std::string& scenePath)
    {
        m_renderSystem.Shutdown(m_world);
        m_physicsDebugRenderSystem.Shutdown();

        if (!m_sceneSerializer.Load(m_world, scenePath, m_shaderPath))
            return false;

        m_scenePath = scenePath;
        m_sceneSavePath = MakeSceneSavePath(m_scenePath);
        BindDemoSceneEntities();
        UWU_ENGINE_INFO("[Gameplay] Scene loaded from editor menu: '{}'", m_scenePath);
        return true;
    }

    void GameplayState::UpdateEditorCamera(const StateContext& ctx, float dt)
    {
        m_cameraSystem.Update(m_world, ctx.input, dt);
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
            m_physicsDebugRenderSystem.Shutdown();
            if (!m_sceneSerializer.Load(m_world, m_sceneSavePath, m_shaderPath))
                m_sceneSerializer.Load(m_world, m_scenePath, m_shaderPath);
            BindDemoSceneEntities();
            ke.Handled = true;
        }

        if (ke.GetKeyCode() == VK_F3)
        {
            m_physicsDebugRenderSystem.ToggleVisible();
            UWU_ENGINE_INFO("[Gameplay] Physics debug draw: {}",
                m_physicsDebugRenderSystem.IsVisible() ? "enabled" : "disabled");
            ke.Handled = true;
        }
    }

    void GameplayState::FixedUpdate(const StateContext& ctx, float fixedDt)
    {
        ApplyControlledPhysicsInput(ctx, fixedDt);
        if (ctx.input && ctx.input->IsActionPressed("PushSphere"))
        {
            if (auto* sphereBody = m_world.GetComponent<RigidbodyComponent>(m_sphereEntity))
            {
                sphereBody->velocity.x += 2.8f;
                sphereBody->velocity.y += 0.35f;
                sphereBody->velocity.z += 1.2f;
                UWU_ENGINE_INFO("[Gameplay] Applied impulse to Physics sphere");
            }
        }
        m_physicsSystem.FixedUpdate(m_world, fixedDt);

        auto* sphereTransform = m_world.GetComponent<TransformComponent>(m_sphereEntity);
        const auto* sphereBody = m_world.GetComponent<RigidbodyComponent>(m_sphereEntity);
        const auto* sphereCollider = m_world.GetComponent<ColliderComponent>(m_sphereEntity);
        if (sphereTransform && sphereBody && sphereCollider && sphereCollider->type == ColliderType::Sphere)
        {
            const float radius = (std::max)(0.0001f, PhysicsSystem::ComputeWorldSphereRadius(*sphereTransform, *sphereCollider));
            sphereTransform->rotationX += sphereBody->velocity.z / radius * fixedDt;
            sphereTransform->rotationZ -= sphereBody->velocity.x / radius * fixedDt;
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
        m_physicsDebugRenderSystem.Render(m_world, ctx.renderer);
    }

    void GameplayState::BuildDemoScene()
    {
        m_renderSystem.Shutdown(m_world);
        m_world.Clear();
        m_controlledEntity = kInvalidEntity;
        m_controlledChildEntity = kInvalidEntity;
        m_spinnerEntity = kInvalidEntity;
        m_modelEntity = kInvalidEntity;
        m_cameraEntity = kInvalidEntity;
        m_platformEntity = kInvalidEntity;
        m_sphereEntity = kInvalidEntity;
        m_boxEntity = kInvalidEntity;

        m_controlledEntity = CreateTriangleEntity(
            "Player triangle",
            TransformComponent{ -0.85f, -0.30f, 0.0f, 0.42f, 0.42f, 1.0f },
            Color4{ 0.95f, 0.15f, 0.18f, 1.0f });

        m_controlledChildEntity = CreateTriangleEntity(
            "Player child triangle",
            TransformComponent{ 1.05f, 0.0f, 0.0f, 0.45f, 0.45f, 1.0f },
            Color4{ 1.0f, 0.95f, 0.25f, 1.0f });

        m_spinnerEntity = CreateTriangleEntity(
            "Auto spinner",
            TransformComponent{ -0.20f, 0.28f, 0.0f, 0.34f, 0.34f, 1.0f },
            Color4{ 0.15f, 0.90f, 0.30f, 1.0f });

        CreateTriangleEntity(
            "Static blue triangle",
            TransformComponent{ 0.15f, -0.42f, 0.0f, 0.30f, 0.30f, 1.0f, 0.0f, 0.0f, 0.35f },
            Color4{ 0.20f, 0.45f, 1.0f, 1.0f });

        m_platformEntity = CreatePrimitiveEntity(
            "Physics platform",
            PrimitiveType::Box,
            TransformComponent{ -0.85f, -0.85f, 0.0f, 1.8f, 0.16f, 2.4f },
            Color4{ 0.35f, 0.36f, 0.40f, 1.0f });
        m_world.AddComponent<RigidbodyComponent>(m_platformEntity, RigidbodyComponent{ {}, {}, 0.0f, false, true });
        m_world.AddComponent<ColliderComponent>(m_platformEntity, ColliderComponent{ ColliderType::Box, Vector3{ 0.5f, 0.5f, 0.5f } });

        const EntityId rampEntity = CreatePrimitiveEntity(
            "Inclined physics ramp",
            PrimitiveType::Box,
            TransformComponent{ 0.80f, -1.22f, 0.0f, 2.4f, 0.14f, 2.4f, 0.0f, 0.0f, -0.35f },
            Color4{ 0.25f, 0.42f, 0.34f, 1.0f });
        m_world.AddComponent<RigidbodyComponent>(rampEntity, RigidbodyComponent{ {}, {}, 0.0f, false, true });
        m_world.AddComponent<ColliderComponent>(rampEntity, ColliderComponent{ ColliderType::Box, Vector3{ 0.5f, 0.5f, 0.5f }, 0.5f, {}, false, 0.0f, 0.08f });

        m_sphereEntity = CreatePrimitiveEntity(
            "Physics sphere",
            PrimitiveType::Sphere,
            TransformComponent{ -0.45f, -0.54f, 0.0f, 0.35f, 0.35f, 0.35f },
            Color4{ 1.0f, 1.0f, 1.0f, 1.0f });
        if (auto* sphereMesh = m_world.GetComponent<MeshRendererComponent>(m_sphereEntity))
        {
            sphereMesh->materialResourcePath = "Assets\\Materials\\CatSphere.material.json";
            sphereMesh->material.shaderPath = L"Assets\\Shaders\\phong_test_light.hlsl";
            sphereMesh->material.texturePath = "Assets\\Textures\\cat.png";
        }
        m_world.AddComponent<RigidbodyComponent>(m_sphereEntity, RigidbodyComponent{ {}, {}, 1.0f, true, false, 0.15f });
        m_world.AddComponent<ColliderComponent>(m_sphereEntity, ColliderComponent{ ColliderType::Sphere, Vector3{ 0.5f, 0.5f, 0.5f }, 0.5f, {}, false, 0.1f, 0.08f });

        m_boxEntity = CreatePrimitiveEntity(
            "Physics box",
            PrimitiveType::Box,
            TransformComponent{ 0.45f, 0.75f, 0.18f, 0.38f, 0.38f, 0.38f },
            Color4{ 1.0f, 0.55f, 0.18f, 1.0f });
        m_world.AddComponent<RigidbodyComponent>(m_boxEntity, RigidbodyComponent{ {}, {}, 1.5f, true, false, 0.2f });
        m_world.AddComponent<ColliderComponent>(m_boxEntity, ColliderComponent{ ColliderType::Box, Vector3{ 0.5f, 0.5f, 0.5f } });
        m_world.AddComponent<PlayerControllerComponent>(m_boxEntity, PlayerControllerComponent{});
        m_controlledEntity = m_boxEntity;

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
        m_modelEntity = FindEntityByTag("Shiba model");
        m_cameraEntity = FindEntityByTag("Main camera");
        m_platformEntity = FindEntityByTag("Physics platform");
        m_sphereEntity = FindEntityByTag("Physics sphere");
        m_boxEntity = FindEntityByTag("Physics box");
        m_controlledEntity = m_modelEntity != kInvalidEntity ? m_modelEntity : m_boxEntity;
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

    EntityId GameplayState::CreatePrimitiveEntity(const std::string& name,
        PrimitiveType primitive, const TransformComponent& transform, const Color4& color)
    {
        EntityId entity = m_world.CreateEntity();
        m_world.AddComponent<TagComponent>(entity, TagComponent{ name });
        m_world.AddComponent<TransformComponent>(entity, transform);

        MaterialDesc material;
        material.baseColor = color;
        material.shaderPath = m_shaderPath;

        m_world.AddComponent<MeshRendererComponent>(
            entity,
            MeshRendererComponent{ MeshFactory::CreatePrimitive(primitive, color), std::move(material) });

        UWU_ENGINE_INFO("[Gameplay] Entity {} created ({})", entity, name);
        return entity;
    }

    EntityId GameplayState::CreateCameraEntity()
    {
        EntityId entity = m_world.CreateEntity();
        m_world.AddComponent<TagComponent>(entity, TagComponent{ "Main camera" });
        m_world.AddComponent<TransformComponent>(
            entity,
            TransformComponent{ 0.0f, 0.15f, -3.8f, 1.0f, 1.0f, 1.0f });

        CameraComponent camera;
        camera.primary = true;
        camera.orthographic = true;
        camera.zoom = 1.0f;
        camera.viewHalfWidth = 2.2f;
        camera.viewHalfHeight = 1.25f;
        camera.fovYRadians = 1.04719755f;
        camera.nearPlane = 0.001f;
        camera.farPlane = 1000.0f;
        camera.moveSpeed = 1.8f;
        camera.zoomSpeed = 1.0f;
        camera.rotateSpeed = 0.010f;
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

        if (auto* box = m_world.GetComponent<TransformComponent>(m_boxEntity))
            box->rotationY += 0.35f * dt;
    }

    void GameplayState::ApplyControlledPhysicsInput(const StateContext& ctx, float /*fixedDt*/)
    {
        if (!ctx.input || m_controlledEntity == kInvalidEntity)
            return;

        auto* transform = m_world.GetComponent<TransformComponent>(m_controlledEntity);
        auto* rigidbody = m_world.GetComponent<RigidbodyComponent>(m_controlledEntity);
        auto* controller = m_world.GetComponent<PlayerControllerComponent>(m_controlledEntity);
        if (!transform || !rigidbody || !controller || rigidbody->isStatic)
            return;

        Vector3 move{ 0.0f, 0.0f, 0.0f };
        if (ctx.input->IsActionDown("PlayerMoveLeft"))     move.x -= 1.0f;
        if (ctx.input->IsActionDown("PlayerMoveRight"))    move.x += 1.0f;
        if (ctx.input->IsActionDown("PlayerMoveForward"))  move.z += 1.0f;
        if (ctx.input->IsActionDown("PlayerMoveBackward")) move.z -= 1.0f;

        if (LengthSquared(move) > 0.0001f)
        {
            move = Normalize(move);
            rigidbody->velocity.x = move.x * controller->moveSpeed;
            rigidbody->velocity.z = move.z * controller->moveSpeed;
            transform->rotationY = std::atan2(move.x, move.z);
        }
        else
        {
            rigidbody->velocity.x = 0.0f;
            rigidbody->velocity.z = 0.0f;
        }

        if (ctx.input->IsActionPressed("PlayerJump") && m_physicsSystem.IsGrounded(m_controlledEntity))
            rigidbody->velocity.y = controller->jumpSpeed;
    }

    void GameplayState::BindInputActions(InputManager* input) const
    {
        if (!input)
            return;

        input->BindAction("PlayerMoveLeft", { VK_LEFT });
        input->BindAction("PlayerMoveRight", { VK_RIGHT });
        input->BindAction("PlayerMoveForward", { VK_UP });
        input->BindAction("PlayerMoveBackward", { VK_DOWN });
        input->BindAction("PlayerJump", { VK_SPACE });
        input->BindAction("PushSphere", { 'F' });
    }

    void GameplayState::SetupPhysicsEventLogging()
    {
        m_physicsSystem.ClearEventListeners();
        m_physicsSystem.AddEventListener(
            [this](const PhysicsEvent& event)
            {
                if (event.kind != PhysicsEventKind::CollisionEnter
                    && event.kind != PhysicsEventKind::TriggerEnter)
                    return;

                const auto* tagA = m_world.GetComponent<TagComponent>(event.contact.a);
                const auto* tagB = m_world.GetComponent<TagComponent>(event.contact.b);
                UWU_ENGINE_INFO("[Physics] {}: {} <-> {}",
                    event.kind == PhysicsEventKind::TriggerEnter ? "Trigger" : "Collision",
                    tagA ? tagA->name : std::to_string(event.contact.a),
                    tagB ? tagB->name : std::to_string(event.contact.b));
            });
    }
}
