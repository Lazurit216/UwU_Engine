#include "uwupch.h"
#include "EditorState.h"

#include "Engine/Logger.h"
#include "Engine/Renderer/MeshGeometry.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuizmo.h"

#if defined(_WIN32)
#include <shellapi.h>
#endif

namespace UwU_Engine
{
    namespace
    {
        bool IsMouseEvent(EventType type)
        {
            return type == EventType::MouseMoved
                || type == EventType::MouseButtonPressed
                || type == EventType::MouseButtonReleased
                || type == EventType::MouseWheel;
        }

        bool IsKeyboardEvent(EventType type)
        {
            return type == EventType::KeyPressed || type == EventType::KeyReleased;
        }

        Vector3 CameraForward(const TransformComponent& transform)
        {
            const Matrix4 rotation = RotationMatrix(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return Normalize(TransformDirection(rotation, Vector3{ 0.0f, 0.0f, 1.0f }));
        }

        Vector3 CameraUp(const TransformComponent& transform)
        {
            const Matrix4 rotation = RotationMatrix(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
            return Normalize(TransformDirection(rotation, Vector3{ 0.0f, 1.0f, 0.0f }));
        }

        Matrix4 BuildTransformMatrix(const TransformComponent& transform)
        {
            Matrix4 model{ 1.0f };
            model = glm::translate(model, Vector3{ transform.x, transform.y, transform.z });
            model *= RotationMatrix(transform.rotationX, transform.rotationY, transform.rotationZ);
            model = glm::scale(model, Vector3{ transform.scaleX, transform.scaleY, transform.scaleZ });
            return model;
        }

        ImGuizmo::OPERATION ToGizmoOperation(int operation)
        {
            switch (operation)
            {
            case 1: return ImGuizmo::ROTATE;
            case 2: return ImGuizmo::SCALE;
            case 0:
            default:
                return ImGuizmo::TRANSLATE;
            }
        }

        std::string EntityDisplayName(const World& world, EntityId entity)
        {
            if (entity == kInvalidEntity)
                return "<none>";

            if (const auto* tag = world.GetComponent<TagComponent>(entity))
                return tag->name;

            return std::format("Entity {}", entity);
        }

        std::string ToAssetLabel(const std::filesystem::path& root, const std::filesystem::path& path)
        {
            std::error_code error;
            const std::filesystem::path relative = std::filesystem::relative(path, root, error);
            return (error ? path.filename() : relative).string();
        }

        bool HasExtension(const std::filesystem::path& path, std::initializer_list<const char*> extensions)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

            for (const char* expected : extensions)
            {
                if (extension == expected)
                    return true;
            }

            return false;
        }

        bool IsMeshAssetPath(const std::filesystem::path& path)
        {
            return HasExtension(path, { ".obj", ".fbx", ".dae", ".gltf", ".glb", ".3ds", ".blend" });
        }

        bool IsMaterialAssetPath(const std::filesystem::path& path)
        {
            const std::string filename = path.filename().string();
            return HasExtension(path, { ".json" })
                && filename.find(".material") != std::string::npos;
        }

        bool IsSceneAssetPath(const std::filesystem::path& path)
        {
            if (!HasExtension(path, { ".json" }))
                return false;

            if (path.filename().string().starts_with("__editor_"))
                return false;

            for (const auto& part : path)
            {
                if (part == "Scenes")
                    return true;
            }

            return false;
        }

        const char* AssetTypeLabel(const std::filesystem::path& path)
        {
            if (IsMeshAssetPath(path)) return "Mesh";
            if (IsMaterialAssetPath(path)) return "Material";
            if (IsSceneAssetPath(path)) return "Scene";
            if (HasExtension(path, { ".png", ".jpg", ".jpeg", ".tga", ".bmp" })) return "Texture";
            if (HasExtension(path, { ".hlsl" })) return "Shader";
            if (HasExtension(path, { ".mtl" })) return "Model material";
            return "File";
        }

        ImVec4 AssetTypeColor(const std::filesystem::path& path)
        {
            if (IsMeshAssetPath(path)) return ImVec4{ 0.70f, 0.86f, 1.00f, 1.0f };
            if (IsMaterialAssetPath(path)) return ImVec4{ 1.00f, 0.78f, 0.42f, 1.0f };
            if (IsSceneAssetPath(path)) return ImVec4{ 0.74f, 1.00f, 0.66f, 1.0f };
            if (HasExtension(path, { ".png", ".jpg", ".jpeg", ".tga", ".bmp" })) return ImVec4{ 0.95f, 0.72f, 1.00f, 1.0f };
            if (HasExtension(path, { ".hlsl" })) return ImVec4{ 0.60f, 0.82f, 1.00f, 1.0f };
            return ImVec4{ 0.84f, 0.86f, 0.90f, 1.0f };
        }

    }

    EditorState::EditorState(std::shared_ptr<GameplayState> gameplayState, std::string scenePath)
        : m_gameplayState(std::move(gameplayState))
        , m_assetsRoot(ResolveAssetsRoot(scenePath))
        , m_currentScenePath(std::filesystem::path(scenePath).lexically_normal())
        , m_playSnapshotPath((m_assetsRoot / "Scenes" / "__editor_play_snapshot.json").lexically_normal())
    {
    }

    void EditorState::OnEnter(const StateContext& ctx)
    {
        if (m_gameplayState)
        {
            m_gameplayState->OnEnter(ctx);
            m_debugDrawEnabled = m_gameplayState->IsPhysicsDebugVisible();
            SelectFirstEntity(m_gameplayState->GetWorld());
        }

        RefreshAssets();
        UWU_ENGINE_INFO("[Editor] Entered");
    }

    void EditorState::OnExit(const StateContext& ctx)
    {
        if (m_gameplayState)
            m_gameplayState->OnExit(ctx);
        UWU_ENGINE_INFO("[Editor] Exited");
    }

    void EditorState::OnPause(const StateContext& ctx)
    {
        if (m_gameplayState)
            m_gameplayState->OnPause(ctx);
    }

    void EditorState::OnResume(const StateContext& ctx)
    {
        if (m_gameplayState)
            m_gameplayState->OnResume(ctx);
    }

    bool EditorState::OnEvent(const StateContext& ctx, Event& e)
    {
        if (ImGui::GetCurrentContext())
        {
            const ImGuiIO& io = ImGui::GetIO();
            if ((IsMouseEvent(e.GetType()) && io.WantCaptureMouse)
                || (IsKeyboardEvent(e.GetType()) && io.WantCaptureKeyboard))
            {
                e.Handled = true;
                return true;
            }
        }

        return m_gameplayState ? m_gameplayState->OnEvent(ctx, e) : false;
    }

    void EditorState::FixedUpdate(const StateContext& ctx, float fixedDt)
    {
        if (m_simulationEnabled && m_gameplayState)
            m_gameplayState->FixedUpdate(ctx, fixedDt);
    }

    StateTransition EditorState::Update(const StateContext& ctx, float dt)
    {
        if (!m_gameplayState)
            return StateTransition::None();

        UpdateStatistics(dt);
        ApplyPendingSceneLoad();
        ApplyPendingEditorOperations(m_gameplayState->GetWorld());
        m_gameplayState->SetPhysicsDebugVisible(m_debugDrawEnabled);

        StateTransition transition = StateTransition::None();
        if (m_simulationEnabled)
            transition = m_gameplayState->Update(ctx, dt);
        else
            m_gameplayState->UpdateEditorCamera(ctx, dt);

        if (m_selectedEntity != kInvalidEntity && !m_gameplayState->GetWorld().IsAlive(m_selectedEntity))
            SelectFirstEntity(m_gameplayState->GetWorld());

        return transition;
    }

    void EditorState::Render(const StateContext& ctx)
    {
        if (m_gameplayState)
            m_gameplayState->Render(ctx);

        if (ImGui::GetCurrentContext())
            DrawEditorUi(ctx);
    }

    void EditorState::DrawEditorUi(const StateContext& ctx)
    {
        DrawDockspace();
        DrawToolbar();

        if (!m_gameplayState)
            return;

        World& world = m_gameplayState->GetWorld();
        DrawSceneWindow(ctx, world);
        DrawHierarchy(world);
        DrawInspector(world);
        DrawAssets();
        DrawConsole();
        DrawStatistics(world);
    }

    void EditorState::DrawDockspace()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImGuiID dockspaceId = ImGui::GetID("UwUEditorDockspace");

        if (!m_dockLayoutBuilt)
        {
            m_dockLayoutBuilt = true;
            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId,
                ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::DockBuilderSetNodePos(dockspaceId, viewport->WorkPos);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

            ImGuiID leftId = 0;
            ImGuiID rightId = 0;
            ImGuiID bottomId = 0;
            ImGuiID centerId = dockspaceId;
            ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, 0.20f, &leftId, &centerId);
            ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, 0.25f, &rightId, &centerId);
            ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.28f, &bottomId, &centerId);
            m_sceneDockNodeId = centerId;

            ImGui::DockBuilderDockWindow("Hierarchy", leftId);
            ImGui::DockBuilderDockWindow("Inspector", rightId);
            ImGui::DockBuilderDockWindow("Assets", bottomId);
            ImGui::DockBuilderDockWindow("Console", bottomId);
            ImGui::DockBuilderDockWindow("Statistics", bottomId);
            ImGui::DockBuilderFinish(dockspaceId);
        }

        ImGui::DockSpaceOverViewport(
            dockspaceId,
            viewport,
            ImGuiDockNodeFlags_PassthruCentralNode);
    }

    void EditorState::DrawToolbar()
    {
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene"))
                SaveCurrentScene();
            DrawLoadSceneMenu();
            if (ImGui::MenuItem("Rescan Assets"))
                RefreshAssets();
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::Button(m_simulationEnabled ? "Edit" : "Play"))
            TogglePlayMode();
        ImGui::SameLine();
        ImGui::Checkbox("Debug Draw", &m_debugDrawEnabled);
        ImGui::SameLine();
        ImGui::Text("Selected: %u", m_selectedEntity);

        ImGui::EndMainMenuBar();
    }

    void EditorState::SaveCurrentScene()
    {
        if (!m_gameplayState)
        {
            UWU_ENGINE_WARN("[Editor] Cannot save scene: gameplay state is unavailable");
            return;
        }

        if (m_currentScenePath.empty())
            m_currentScenePath = (m_assetsRoot / "Scenes" / "scene_test_saved.json").lexically_normal();

        if (m_gameplayState->SaveSceneToFile(m_currentScenePath.string()))
        {
            UWU_ENGINE_INFO("[Editor] Scene saved to '{}'", m_currentScenePath.string());
            RefreshAssets();
        }
        else
        {
            UWU_ENGINE_WARN("[Editor] Failed to save scene '{}'", m_currentScenePath.string());
        }
    }

    void EditorState::TogglePlayMode()
    {
        if (!m_gameplayState)
            return;

        if (!m_simulationEnabled)
        {
            if (m_gameplayState->SaveSceneToFile(m_playSnapshotPath.string()))
            {
                m_simulationEnabled = true;
                UWU_ENGINE_INFO("[Editor] Play mode enabled");
            }
            return;
        }

        m_simulationEnabled = false;
        m_pendingSceneLoadPath = m_playSnapshotPath;
        m_pendingSceneLoad = true;
        m_restoreEditAfterSceneLoad = true;
        UWU_ENGINE_INFO("[Editor] Returning to edit mode");
    }

    void EditorState::DrawLoadSceneMenu()
    {
        if (!ImGui::BeginMenu("Load Scene"))
            return;

        bool hasScenes = false;
        for (const auto& path : m_assetEntries)
        {
            if (!IsSceneAssetPath(path))
                continue;

            hasScenes = true;
            const std::string label = ToAssetLabel(m_assetsRoot, path);
            const bool selected = path.lexically_normal() == m_currentScenePath;
            if (ImGui::MenuItem(label.c_str(), nullptr, selected))
                QueueSceneLoad(path);
        }

        if (!hasScenes)
            ImGui::MenuItem("No scene files found", nullptr, false, false);

        ImGui::EndMenu();
    }

    void EditorState::DrawSceneWindow(const StateContext& ctx, World& world)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 scenePos = viewport->WorkPos;
        ImVec2 sceneSize = viewport->WorkSize;

        if (ImGuiDockNode* sceneNode = ImGui::DockBuilderGetNode(m_sceneDockNodeId))
        {
            scenePos = sceneNode->Pos;
            sceneSize = sceneNode->Size;
        }

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        drawList->AddRect(
            scenePos,
            ImVec2(scenePos.x + sceneSize.x, scenePos.y + sceneSize.y),
            IM_COL32(85, 95, 110, 160));
        drawList->AddText(
            ImVec2(scenePos.x + 8.0f, scenePos.y + 8.0f),
            IM_COL32(210, 215, 225, 220),
            "Scene");

        const ImGuiWindowFlags overlayFlags =
            ImGuiWindowFlags_NoDocking
            | ImGuiWindowFlags_NoDecoration
            | ImGuiWindowFlags_AlwaysAutoResize
            | ImGuiWindowFlags_NoSavedSettings;

        ImGui::SetNextWindowPos(ImVec2(scenePos.x + 8.0f, scenePos.y + 28.0f), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.78f);
        ImGui::Begin("Scene Controls", nullptr, overlayFlags);

        ImGui::RadioButton("Move", &m_gizmoOperation, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Rotate", &m_gizmoOperation, 1);
        ImGui::SameLine();
        ImGui::RadioButton("Scale", &m_gizmoOperation, 2);
        ImGui::SameLine();
        ImGui::RadioButton("Local", &m_gizmoMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("World", &m_gizmoMode, 1);
        ImGui::SameLine();
        ImGui::Checkbox("Snap", &m_gizmoSnap);
        if (m_gizmoSnap)
        {
            ImGui::SameLine();
            ImGui::SetNextItemWidth(140.0f);
            ImGui::DragFloat3("##SnapValues", m_gizmoSnapValues, 0.01f, 0.001f, 100.0f);
        }

        ImGui::End();

        DrawSelectedGizmo(ctx, world, scenePos, sceneSize);
    }

    void EditorState::DrawSelectedGizmo(const StateContext& ctx, World& world, const ImVec2& scenePos, const ImVec2& sceneSize)
    {
        if (sceneSize.x <= 1.0f || sceneSize.y <= 1.0f)
            return;
        if (m_selectedEntity == kInvalidEntity || !world.IsAlive(m_selectedEntity))
            return;

        auto* selectedTransform = world.GetComponent<TransformComponent>(m_selectedEntity);
        if (!selectedTransform)
            return;

        TransformComponent cameraTransform;
        CameraComponent camera;
        bool cameraFound = false;
        world.ForEach<TransformComponent, CameraComponent>(
            [&cameraTransform, &camera, &cameraFound](EntityId /*entity*/, TransformComponent& transform, CameraComponent& cameraComponent)
            {
                if (!cameraFound && cameraComponent.primary)
                {
                    cameraTransform = transform;
                    camera = cameraComponent;
                    cameraFound = true;
                }
            });
        if (!cameraFound)
            return;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 renderPos = viewport->Pos;
        const ImVec2 renderSize = ctx.renderer
            ? ImVec2(static_cast<float>(ctx.renderer->GetWidth()), static_cast<float>(ctx.renderer->GetHeight()))
            : viewport->Size;

        const float aspect = renderSize.y > 0.0f ? renderSize.x / renderSize.y : 1.0f;
        const float zoom = (std::max)(camera.zoom, 0.01f);
        float halfWidth = (std::max)(camera.viewHalfWidth, 0.01f);
        float halfHeight = (std::max)(camera.viewHalfHeight, 0.01f);
        const float designAspect = halfWidth / halfHeight;
        if (aspect >= designAspect)
            halfWidth = halfHeight * aspect;
        else
            halfHeight = halfWidth / (std::max)(aspect, 0.01f);

        halfWidth /= zoom;
        halfHeight /= zoom;

        const Vector3 eye{ cameraTransform.x, cameraTransform.y, cameraTransform.z };
        const Matrix4 view = glm::lookAtLH(
            eye,
            eye + CameraForward(cameraTransform),
            CameraUp(cameraTransform));
        const Matrix4 projection = camera.orthographic
            ? glm::orthoLH_ZO(-halfWidth, halfWidth, -halfHeight, halfHeight,
                (std::max)(camera.nearPlane, 0.001f),
                (std::max)(camera.farPlane, camera.nearPlane + 1.0f))
            : glm::perspectiveLH_ZO(
                camera.fovYRadians,
                aspect,
                (std::max)(camera.nearPlane, 0.001f),
                (std::max)(camera.farPlane, camera.nearPlane + 1.0f));

        Matrix4 model = BuildTransformMatrix(*selectedTransform);

        ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
        ImGuizmo::SetRect(renderPos.x, renderPos.y, renderSize.x, renderSize.y);
        ImGuizmo::SetOrthographic(camera.orthographic);
        ImGuizmo::Manipulate(
            glm::value_ptr(view),
            glm::value_ptr(projection),
            ToGizmoOperation(m_gizmoOperation),
            m_gizmoMode == 0 ? ImGuizmo::LOCAL : ImGuizmo::WORLD,
            glm::value_ptr(model),
            nullptr,
            m_gizmoSnap ? m_gizmoSnapValues : nullptr);

        if (!ImGuizmo::IsUsing())
            return;

        float translation[3] = {};
        float rotationDegrees[3] = {};
        float scale[3] = {};
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(model),
            translation,
            rotationDegrees,
            scale);

        selectedTransform->x = translation[0];
        selectedTransform->y = translation[1];
        selectedTransform->z = translation[2];
        selectedTransform->rotationX = glm::radians(rotationDegrees[0]);
        selectedTransform->rotationY = glm::radians(rotationDegrees[1]);
        selectedTransform->rotationZ = glm::radians(rotationDegrees[2]);
        selectedTransform->scaleX = scale[0];
        selectedTransform->scaleY = scale[1];
        selectedTransform->scaleZ = scale[2];
    }

    void EditorState::DrawHierarchy(World& world)
    {
        ImGui::Begin("Hierarchy");

        if (ImGui::Button("Create Empty"))
        {
            const EntityId entity = world.CreateEntity();
            world.AddComponent<TagComponent>(entity, TagComponent{ "New Entity" });
            world.AddComponent<TransformComponent>(entity);
            m_selectedEntity = entity;
        }

        ImGui::Separator();
        for (EntityId entity : world.GetEntities())
        {
            const auto* hierarchy = world.GetComponent<HierarchyComponent>(entity);
            if (!hierarchy || hierarchy->parent == kInvalidEntity)
                DrawEntityNode(world, entity);
        }

        ImGui::End();
    }

    void EditorState::DrawEntityNode(World& world, EntityId entity)
    {
        const auto* hierarchy = world.GetComponent<HierarchyComponent>(entity);
        const std::string label = std::format("{}##entity_{}",
            EntityDisplayName(world, entity),
            entity);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        if (m_selectedEntity == entity)
            flags |= ImGuiTreeNodeFlags_Selected;
        if (!hierarchy || hierarchy->children.empty())
            flags |= ImGuiTreeNodeFlags_Leaf;

        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        if (ImGui::IsItemClicked())
            m_selectedEntity = entity;

        if (open)
        {
            if (hierarchy)
            {
                for (EntityId child : hierarchy->children)
                {
                    if (world.IsAlive(child))
                        DrawEntityNode(world, child);
                }
            }
            ImGui::TreePop();
        }
    }

    void EditorState::DrawInspector(World& world)
    {
        ImGui::Begin("Inspector");

        if (m_selectedEntity == kInvalidEntity || !world.IsAlive(m_selectedEntity))
        {
            ImGui::TextUnformatted("No entity selected");
            ImGui::End();
            return;
        }

        if (auto* tag = world.GetComponent<TagComponent>(m_selectedEntity))
        {
            std::array<char, 128> buffer{};
            const size_t copyLength = (std::min)(buffer.size() - 1, tag->name.size());
            std::copy_n(tag->name.c_str(), copyLength, buffer.data());
            if (ImGui::InputText("Name", buffer.data(), buffer.size()))
                tag->name = buffer.data();
        }
        else if (ImGui::Button("Add Tag"))
        {
            world.AddComponent<TagComponent>(m_selectedEntity, TagComponent{ "Entity" });
        }

        ImGui::Separator();
        if (auto* transform = world.GetComponent<TransformComponent>(m_selectedEntity))
            DrawTransform(*transform);

        if (auto* mesh = world.GetComponent<MeshRendererComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Vertices: %zu", mesh->mesh.vertices.size());
                ImGui::Text("Indices: %zu", mesh->mesh.indices.size());
                ImGui::Text("Mesh: %s", mesh->meshResourcePath.empty() ? "<embedded>" : mesh->meshResourcePath.c_str());
                ImGui::Text("Material: %s", mesh->materialResourcePath.empty() ? "<embedded>" : mesh->materialResourcePath.c_str());
                DrawMeshAssetPicker(m_selectedEntity, *mesh);
                DrawMaterialAssetPicker(m_selectedEntity, *mesh);
                if (ImGui::ColorEdit4("Base Color", &mesh->material.baseColor.r))
                    ApplyMeshRendererColor(m_selectedEntity, *mesh);
                if (ImGui::Button("Remove Mesh Renderer"))
                    QueueRemoveMeshRenderer(m_selectedEntity);
            }
        }

        if (auto* camera = world.GetComponent<CameraComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Primary", &camera->primary);
                ImGui::Checkbox("Orthographic", &camera->orthographic);
                ImGui::DragFloat("Zoom", &camera->zoom, 0.01f, 0.05f, 20.0f);
                ImGui::DragFloat("Near", &camera->nearPlane, 0.01f, 0.001f, camera->farPlane);
                ImGui::DragFloat("Far", &camera->farPlane, 1.0f, camera->nearPlane + 0.01f, 5000.0f);
                if (ImGui::Button("Remove Camera"))
                    world.RemoveComponent<CameraComponent>(m_selectedEntity);
            }
        }

        if (auto* body = world.GetComponent<RigidbodyComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Velocity", &body->velocity.x, 0.01f);
                ImGui::DragFloat("Mass", &body->mass, 0.05f, 0.0f, 100.0f);
                bool useGravity = body->useGravity;
                if (ImGui::Checkbox("Use Gravity", &useGravity))
                {
                    body->useGravity = useGravity;
                    if (!body->useGravity)
                    {
                        body->velocity.y = 0.0f;
                        body->acceleration.y = 0.0f;
                    }
                }
                ImGui::Checkbox("Static", &body->isStatic);
                ImGui::DragFloat("Damping", &body->linearDamping, 0.01f, 0.0f, 10.0f);
                if (ImGui::Button("Remove Rigidbody"))
                    world.RemoveComponent<RigidbodyComponent>(m_selectedEntity);
            }
        }

        if (auto* collider = world.GetComponent<ColliderComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen))
            {
                int type = static_cast<int>(collider->type);
                const char* types[] = { "Box", "Sphere" };
                if (ImGui::Combo("Type", &type, types, IM_ARRAYSIZE(types)))
                    collider->type = static_cast<ColliderType>(type);
                ImGui::DragFloat3("Half Extents", &collider->halfExtents.x, 0.01f, 0.01f, 100.0f);
                ImGui::DragFloat("Radius", &collider->radius, 0.01f, 0.01f, 100.0f);
                ImGui::DragFloat3("Offset", &collider->offset.x, 0.01f);
                ImGui::Checkbox("Trigger", &collider->isTrigger);
                ImGui::DragFloat("Bounciness", &collider->bounciness, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Friction", &collider->friction, 0.01f, 0.0f, 1.0f);
                if (ImGui::Button("Remove Collider"))
                    world.RemoveComponent<ColliderComponent>(m_selectedEntity);
            }
        }

        if (auto* controller = world.GetComponent<PlayerControllerComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Player Controller", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("Move Speed", &controller->moveSpeed, 0.01f, 0.0f, 20.0f);
                ImGui::DragFloat("Jump Speed", &controller->jumpSpeed, 0.01f, 0.0f, 30.0f);
                if (ImGui::Button("Remove Player Controller"))
                    world.RemoveComponent<PlayerControllerComponent>(m_selectedEntity);
            }
        }

        if (auto* hierarchy = world.GetComponent<HierarchyComponent>(m_selectedEntity))
            DrawHierarchyComponent(world, m_selectedEntity, *hierarchy);

        DrawComponentButtons(world, m_selectedEntity);

        ImGui::Separator();
        if (ImGui::Button("Delete Entity"))
        {
            QueueDestroyEntity(m_selectedEntity);
            m_selectedEntity = kInvalidEntity;
        }

        ImGui::End();
    }

    void EditorState::DrawTransform(TransformComponent& transform)
    {
        if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        ImGui::DragFloat3("Position", &transform.x, 0.01f);
        ImGui::DragFloat3("Scale", &transform.scaleX, 0.01f, 0.001f, 100.0f);
        ImGui::DragFloat3("Rotation", &transform.rotationX, 0.01f);
    }

    void EditorState::DrawComponentButtons(World& world, EntityId entity)
    {
        if (ImGui::Button("Add Component"))
            ImGui::OpenPopup("AddComponentPopup");

        if (ImGui::BeginPopup("AddComponentPopup"))
        {
            const bool hasTransform = world.HasComponent<TransformComponent>(entity);
            const bool hasMeshRenderer = world.HasComponent<MeshRendererComponent>(entity);
            const bool hasRigidbody = world.HasComponent<RigidbodyComponent>(entity);
            const bool hasCollider = world.HasComponent<ColliderComponent>(entity);
            const bool hasCamera = world.HasComponent<CameraComponent>(entity);
            const bool hasController = world.HasComponent<PlayerControllerComponent>(entity);
            const bool hasHierarchy = world.HasComponent<HierarchyComponent>(entity);

            if (ImGui::MenuItem(hasTransform ? "Transform already exists" : "Add Transform", nullptr, false, !hasTransform))
                EnsureTransform(world, entity);
            if (ImGui::MenuItem(hasMeshRenderer ? "Mesh Renderer already exists" : "Add Mesh Renderer", nullptr, false, !hasMeshRenderer))
                AddDefaultMeshRenderer(world, entity);
            if (ImGui::MenuItem(hasRigidbody ? "Rigidbody already exists" : "Add Rigidbody", nullptr, false, !hasRigidbody))
            {
                EnsureTransform(world, entity);
                world.AddComponent<RigidbodyComponent>(entity);
            }
            if (ImGui::MenuItem(hasCollider ? "Collider already exists" : "Add Collider", nullptr, false, !hasCollider))
            {
                EnsureTransform(world, entity);
                world.AddComponent<ColliderComponent>(entity);
            }
            if (ImGui::MenuItem(hasCamera ? "Camera already exists" : "Add Camera", nullptr, false, !hasCamera))
            {
                EnsureTransform(world, entity);
                world.AddComponent<CameraComponent>(entity);
            }
            if (ImGui::MenuItem(hasController ? "Player Controller already exists" : "Add Player Controller", nullptr, false, !hasController))
            {
                EnsureTransform(world, entity);
                if (!world.HasComponent<RigidbodyComponent>(entity))
                    world.AddComponent<RigidbodyComponent>(entity);
                world.AddComponent<PlayerControllerComponent>(entity);
            }
            if (ImGui::MenuItem(hasHierarchy ? "Hierarchy already exists" : "Add Hierarchy", nullptr, false, !hasHierarchy))
                world.AddComponent<HierarchyComponent>(entity);
            ImGui::EndPopup();
        }
    }

    void EditorState::DrawHierarchyComponent(World& world, EntityId entity, HierarchyComponent& hierarchy)
    {
        if (!ImGui::CollapsingHeader("Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        const std::string parentName = EntityDisplayName(world, hierarchy.parent);
        ImGui::Text("Parent: %s", parentName.c_str());
        if (ImGui::BeginCombo("Set Parent", parentName.c_str()))
        {
            if (ImGui::Selectable("<none>", hierarchy.parent == kInvalidEntity))
                SetEntityParent(world, entity, kInvalidEntity);

            for (EntityId candidate : world.GetEntities())
            {
                if (candidate == entity || IsDescendantOf(world, candidate, entity))
                    continue;

                const std::string candidateName = EntityDisplayName(world, candidate);
                if (ImGui::Selectable(candidateName.c_str(), hierarchy.parent == candidate))
                    SetEntityParent(world, entity, candidate);
            }
            ImGui::EndCombo();
        }

        size_t aliveChildren = 0;
        const std::vector<EntityId> children = hierarchy.children;
        for (EntityId child : children)
            if (world.IsAlive(child))
                ++aliveChildren;
        ImGui::Text("Children: %zu", aliveChildren);

        for (EntityId child : hierarchy.children)
        {
            if (!world.IsAlive(child))
                continue;

            const std::string childName = EntityDisplayName(world, child);
            ImGui::BulletText("%s", childName.c_str());
            ImGui::SameLine();
            ImGui::PushID(static_cast<int>(child));
            if (ImGui::SmallButton("Select"))
                m_selectedEntity = child;
            ImGui::SameLine();
            if (ImGui::SmallButton("Unparent"))
                SetEntityParent(world, child, kInvalidEntity);
            ImGui::PopID();
        }

        if (ImGui::Button("Remove Hierarchy"))
            RemoveHierarchyComponent(world, entity);
    }

    void EditorState::DrawMeshAssetPicker(EntityId entity, MeshRendererComponent& mesh)
    {
        if (ImGui::Button("Select Mesh"))
            ImGui::OpenPopup("MeshAssetPicker");

        if (!ImGui::BeginPopup("MeshAssetPicker"))
            return;

        if (ImGui::Selectable("<embedded>", mesh.meshResourcePath.empty()))
        {
            mesh.meshResourcePath.clear();
            mesh.meshResource.reset();
            mesh.meshResourceApplied = false;
            mesh.meshLoadFailedLogged = false;
            mesh.placeholderMeshApplied = false;
            QueueResetMeshRendererDrawable(entity);
        }

        for (const auto& path : m_assetEntries)
        {
            if (!IsMeshAssetPath(path))
                continue;

            const std::string label = ToAssetLabel(m_assetsRoot, path);
            const std::string normalizedPath = path.lexically_normal().string();
            if (ImGui::Selectable(label.c_str(), mesh.meshResourcePath == normalizedPath))
                SetMeshResourcePath(entity, mesh, path);
        }
        ImGui::EndPopup();
    }

    void EditorState::DrawMaterialAssetPicker(EntityId entity, MeshRendererComponent& mesh)
    {
        ImGui::SameLine();
        if (ImGui::Button("Select Material"))
            ImGui::OpenPopup("MaterialAssetPicker");

        if (!ImGui::BeginPopup("MaterialAssetPicker"))
            return;

        if (ImGui::Selectable("<embedded>", mesh.materialResourcePath.empty()))
        {
            mesh.materialResourcePath.clear();
            mesh.materialResource.reset();
            mesh.materialResourceApplied = false;
            mesh.materialLoadFailedLogged = false;
            QueueResetMeshRendererDrawable(entity);
        }

        for (const auto& path : m_assetEntries)
        {
            if (!IsMaterialAssetPath(path))
                continue;

            const std::string label = ToAssetLabel(m_assetsRoot, path);
            const std::string normalizedPath = path.lexically_normal().string();
            if (ImGui::Selectable(label.c_str(), mesh.materialResourcePath == normalizedPath))
                SetMaterialResourcePath(entity, mesh, path);
        }
        ImGui::EndPopup();
    }

    void EditorState::AddDefaultMeshRenderer(World& world, EntityId entity)
    {
        EnsureTransform(world, entity);

        MaterialDesc material;
        material.baseColor = Color4{ 0.70f, 0.82f, 1.0f, 1.0f };
        material.shaderPath = FindFallbackShaderPath(world);

        world.AddComponent<MeshRendererComponent>(
            entity,
            MeshRendererComponent{
                MeshFactory::CreateBox(0.4f, 0.4f, 0.4f, material.baseColor),
                std::move(material)
            });
    }

    void EditorState::ApplyMeshRendererColor(EntityId entity, MeshRendererComponent& mesh)
    {
        for (Vertex& vertex : mesh.mesh.vertices)
        {
            vertex.color[0] = mesh.material.baseColor.r;
            vertex.color[1] = mesh.material.baseColor.g;
            vertex.color[2] = mesh.material.baseColor.b;
        }

        QueueResetMeshRendererDrawable(entity);
    }

    void EditorState::ResetMeshRendererDrawable(MeshRendererComponent& mesh)
    {
        if (mesh.drawable)
        {
            mesh.drawable->Shutdown();
            mesh.drawable.reset();
        }
        mesh.initFailedLogged = false;
    }

    void EditorState::SetMeshResourcePath(EntityId entity, MeshRendererComponent& mesh, const std::filesystem::path& path)
    {
        mesh.meshResourcePath = path.lexically_normal().string();
        mesh.meshResource.reset();
        mesh.meshResourceApplied = false;
        mesh.meshLoadFailedLogged = false;
        mesh.placeholderMeshApplied = false;
        QueueResetMeshRendererDrawable(entity);
    }

    void EditorState::SetMaterialResourcePath(EntityId entity, MeshRendererComponent& mesh, const std::filesystem::path& path)
    {
        mesh.materialResourcePath = path.lexically_normal().string();
        mesh.materialResource.reset();
        mesh.materialResourceApplied = false;
        mesh.materialLoadFailedLogged = false;
        mesh.material.texturePath.clear();
        mesh.material.textureWidth = 0;
        mesh.material.textureHeight = 0;
        mesh.material.textureChannels = 0;
        mesh.material.texturePixels.clear();
        mesh.textureResource.reset();
        mesh.textureResourceApplied = false;
        mesh.textureLoadFailedLogged = false;
        mesh.shaderResource.reset();
        mesh.shaderResourceApplied = false;
        mesh.shaderLoadFailedLogged = false;
        mesh.meshResourceApplied = false;
        QueueResetMeshRendererDrawable(entity);
    }

    void EditorState::SetEntityParent(World& world, EntityId entity, EntityId parent)
    {
        if (entity == kInvalidEntity || !world.IsAlive(entity) || entity == parent)
            return;
        if (parent != kInvalidEntity && (!world.IsAlive(parent) || IsDescendantOf(world, parent, entity)))
            return;

        HierarchyComponent* hierarchy = world.GetComponent<HierarchyComponent>(entity);
        if (!hierarchy)
            hierarchy = &world.AddComponent<HierarchyComponent>(entity);

        if (auto* oldParentHierarchy = world.GetComponent<HierarchyComponent>(hierarchy->parent))
        {
            auto& siblings = oldParentHierarchy->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
        }

        hierarchy->parent = parent;
        if (parent == kInvalidEntity)
            return;

        HierarchyComponent* parentHierarchy = world.GetComponent<HierarchyComponent>(parent);
        if (!parentHierarchy)
            parentHierarchy = &world.AddComponent<HierarchyComponent>(parent);

        if (std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), entity) == parentHierarchy->children.end())
            parentHierarchy->children.push_back(entity);
    }

    void EditorState::RemoveHierarchyComponent(World& world, EntityId entity)
    {
        auto* hierarchy = world.GetComponent<HierarchyComponent>(entity);
        if (!hierarchy)
            return;

        if (auto* parentHierarchy = world.GetComponent<HierarchyComponent>(hierarchy->parent))
        {
            auto& siblings = parentHierarchy->children;
            siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
        }

        const std::vector<EntityId> children = hierarchy->children;
        for (EntityId child : children)
        {
            if (auto* childHierarchy = world.GetComponent<HierarchyComponent>(child))
                childHierarchy->parent = kInvalidEntity;
        }

        world.RemoveComponent<HierarchyComponent>(entity);
    }

    void EditorState::DestroyEntityWithChildren(World& world, EntityId entity)
    {
        if (entity == kInvalidEntity || !world.IsAlive(entity))
            return;

        std::vector<EntityId> children;
        if (auto* hierarchy = world.GetComponent<HierarchyComponent>(entity))
            children = hierarchy->children;

        for (EntityId child : children)
            DestroyEntityWithChildren(world, child);

        RemoveHierarchyComponent(world, entity);
        world.DestroyEntity(entity);
    }

    void EditorState::EnsureTransform(World& world, EntityId entity)
    {
        if (!world.HasComponent<TransformComponent>(entity))
            world.AddComponent<TransformComponent>(entity);
    }

    std::wstring EditorState::FindFallbackShaderPath(World& world) const
    {
        std::wstring shaderPath = L"Assets\\Shaders\\Color.hlsl";
        world.ForEach<MeshRendererComponent>(
            [&shaderPath](EntityId /*entity*/, MeshRendererComponent& mesh)
            {
                if (shaderPath == L"Assets\\Shaders\\Color.hlsl" && !mesh.material.shaderPath.empty())
                    shaderPath = mesh.material.shaderPath;
            });
        return shaderPath;
    }

    bool EditorState::IsDescendantOf(const World& world, EntityId entity, EntityId possibleAncestor) const
    {
        EntityId current = entity;
        for (int depth = 0; depth < 64; ++depth)
        {
            const auto* hierarchy = world.GetComponent<HierarchyComponent>(current);
            if (!hierarchy || hierarchy->parent == kInvalidEntity)
                return false;

            if (hierarchy->parent == possibleAncestor)
                return true;

            current = hierarchy->parent;
        }

        return true;
    }

    void EditorState::QueueDestroyEntity(EntityId entity)
    {
        if (entity != kInvalidEntity)
            m_pendingEntityDeletes.push_back(entity);
    }

    void EditorState::QueueRemoveMeshRenderer(EntityId entity)
    {
        if (entity != kInvalidEntity)
            m_pendingMeshRendererRemovals.push_back(entity);
    }

    void EditorState::QueueResetMeshRendererDrawable(EntityId entity)
    {
        if (entity == kInvalidEntity)
            return;

        if (std::find(m_pendingMeshRendererDrawableResets.begin(), m_pendingMeshRendererDrawableResets.end(), entity)
            == m_pendingMeshRendererDrawableResets.end())
        {
            m_pendingMeshRendererDrawableResets.push_back(entity);
        }
    }

    void EditorState::ApplyPendingEditorOperations(World& world)
    {
        if (!m_pendingMeshRendererDrawableResets.empty())
        {
            for (EntityId entity : m_pendingMeshRendererDrawableResets)
            {
                if (auto* mesh = world.GetComponent<MeshRendererComponent>(entity))
                    ResetMeshRendererDrawable(*mesh);
            }
            m_pendingMeshRendererDrawableResets.clear();
        }

        if (!m_pendingMeshRendererRemovals.empty())
        {
            for (EntityId entity : m_pendingMeshRendererRemovals)
            {
                if (world.IsAlive(entity))
                    world.RemoveComponent<MeshRendererComponent>(entity);
            }
            m_pendingMeshRendererRemovals.clear();
        }

        if (!m_pendingEntityDeletes.empty())
        {
            for (EntityId entity : m_pendingEntityDeletes)
                DestroyEntityWithChildren(world, entity);
            m_pendingEntityDeletes.clear();

            if (m_selectedEntity == kInvalidEntity || !world.IsAlive(m_selectedEntity))
                SelectFirstEntity(world);
        }
    }

    void EditorState::UpdateStatistics(float dt)
    {
        m_lastFrameTimeMs = dt * 1000.0f;
        m_statsTimer += dt;
        ++m_statsFrameCount;

        if (m_statsTimer >= 1.0f)
        {
            m_averageFps = m_statsFrameCount / m_statsTimer;
            m_statsTimer = 0.0f;
            m_statsFrameCount = 0;
        }
    }

    void EditorState::DrawAssets()
    {
        ImGui::Begin("Assets");

        if (ImGui::Button("Rescan Assets"))
            RefreshAssets();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Re-read the Assets folder from disk");
        ImGui::SameLine();
        ImGui::Text("%s", m_assetsRoot.string().c_str());
        ImGui::Separator();

        if (!std::filesystem::exists(m_assetsRoot))
        {
            ImGui::TextDisabled("Assets folder is unavailable");
            ImGui::End();
            return;
        }

        ImGui::BeginChild("AssetBrowser", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        const ImGuiTableFlags tableFlags =
            ImGuiTableFlags_RowBg
            | ImGuiTableFlags_BordersInnerV
            | ImGuiTableFlags_Resizable
            | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("AssetBrowserTable", 2, tableFlags))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.72f);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.28f);
            ImGui::TableHeadersRow();
            DrawAssetDirectory(m_assetsRoot);
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::End();
    }

    void EditorState::DrawAssetDirectory(const std::filesystem::path& directory)
    {
        std::vector<std::filesystem::directory_entry> entries;
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(directory))
                entries.push_back(entry);
        }
        catch (const std::exception&)
        {
            return;
        }

        std::sort(entries.begin(), entries.end(),
            [](const std::filesystem::directory_entry& left, const std::filesystem::directory_entry& right)
            {
                if (left.is_directory() != right.is_directory())
                    return left.is_directory();
                return left.path().filename().string() < right.path().filename().string();
            });

        for (const auto& entry : entries)
        {
            const std::filesystem::path path = entry.path().lexically_normal();
            if (entry.is_directory())
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::PushID(path.string().c_str());
                const bool open = ImGui::TreeNodeEx(
                    path.filename().string().c_str(),
                    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextDisabled("Folder");
                if (open)
                {
                    DrawAssetDirectory(path);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            else if (entry.is_regular_file())
            {
                DrawAssetFile(path);
            }
        }
    }

    void EditorState::DrawAssetFile(const std::filesystem::path& path)
    {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        const ImVec4 color = AssetTypeColor(path);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        if (ImGui::Selectable(path.filename().string().c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
            OpenAssetFile(path);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", path.string().c_str());

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("%s", AssetTypeLabel(path));
    }

    void EditorState::DrawStatistics(World& world)
    {
        size_t meshRendererCount = 0;
        size_t colliderCount = 0;
        size_t rigidbodyCount = 0;
        size_t cameraCount = 0;

        world.ForEach<MeshRendererComponent>(
            [&meshRendererCount](EntityId /*entity*/, MeshRendererComponent& /*mesh*/)
            {
                ++meshRendererCount;
            });
        world.ForEach<ColliderComponent>(
            [&colliderCount](EntityId /*entity*/, ColliderComponent& /*collider*/)
            {
                ++colliderCount;
            });
        world.ForEach<RigidbodyComponent>(
            [&rigidbodyCount](EntityId /*entity*/, RigidbodyComponent& /*body*/)
            {
                ++rigidbodyCount;
            });
        world.ForEach<CameraComponent>(
            [&cameraCount](EntityId /*entity*/, CameraComponent& /*camera*/)
            {
                ++cameraCount;
            });

        ImGui::Begin("Statistics");
        ImGui::Text("FPS: %.1f", m_averageFps);
        ImGui::Text("Frame time: %.3f ms", m_lastFrameTimeMs);
        ImGui::Separator();
        ImGui::Text("Entities: %zu", world.EntityCount());
        ImGui::Text("Mesh renderers: %zu", meshRendererCount);
        ImGui::Text("Rigidbodies: %zu", rigidbodyCount);
        ImGui::Text("Colliders: %zu", colliderCount);
        ImGui::Text("Cameras: %zu", cameraCount);
        ImGui::Text("Active physics contacts: %zu",
            m_gameplayState ? m_gameplayState->GetPhysicsContactCount() : 0);
        ImGui::End();
    }

    void EditorState::DrawConsole()
    {
        ImGui::Begin("Console");

        ImGui::Checkbox("Trace", &m_showTraceLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Info", &m_showInfoLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Warn", &m_showWarnLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Error", &m_showErrorLogs);
        ImGui::SameLine();
        ImGui::Checkbox("Events", &m_showEventLogs);

        ImGui::Separator();
        ImGui::BeginChild("ConsoleScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        const auto entries = Logger::GetEntries();
        for (const Logger::Entry& entry : entries)
        {
            bool visible = false;
            ImVec4 color{ 0.85f, 0.85f, 0.85f, 1.0f };
            switch (entry.level)
            {
            case Logger::Level::Trace:
                visible = m_showTraceLogs;
                color = ImVec4(0.60f, 0.62f, 0.66f, 1.0f);
                break;
            case Logger::Level::Info:
                visible = m_showInfoLogs;
                color = ImVec4(0.78f, 0.86f, 1.00f, 1.0f);
                break;
            case Logger::Level::Warn:
                visible = m_showWarnLogs;
                color = ImVec4(1.00f, 0.78f, 0.30f, 1.0f);
                break;
            case Logger::Level::Error:
                visible = m_showErrorLogs;
                color = ImVec4(1.00f, 0.35f, 0.32f, 1.0f);
                break;
            case Logger::Level::Event:
                visible = m_showEventLogs;
                color = ImVec4(0.48f, 0.82f, 0.68f, 1.0f);
                break;
            }

            if (!visible)
                continue;

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(entry.formattedLine.c_str());
            ImGui::PopStyleColor();
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    void EditorState::RefreshAssets()
    {
        m_assetEntries.clear();
        if (m_assetsRoot.empty() || !std::filesystem::exists(m_assetsRoot))
            return;

        try
        {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(m_assetsRoot))
            {
                if (entry.is_regular_file())
                    m_assetEntries.push_back(entry.path().lexically_normal());
            }
            std::sort(m_assetEntries.begin(), m_assetEntries.end());
        }
        catch (const std::exception& e)
        {
            UWU_ENGINE_WARN("[Editor] Asset refresh failed: {}", e.what());
        }
    }

    void EditorState::QueueSceneLoad(const std::filesystem::path& path)
    {
        m_pendingSceneLoadPath = path.lexically_normal();
        m_pendingSceneLoad = true;
        m_restoreEditAfterSceneLoad = false;
        m_simulationEnabled = false;
    }

    void EditorState::ApplyPendingSceneLoad()
    {
        if (!m_pendingSceneLoad || !m_gameplayState)
            return;

        const std::filesystem::path scenePath = m_pendingSceneLoadPath;
        m_pendingSceneLoad = false;

        if (!m_gameplayState->LoadSceneFromFile(scenePath.string()))
        {
            UWU_ENGINE_WARN("[Editor] Failed to load scene '{}'", scenePath.string());
            return;
        }

        if (!m_restoreEditAfterSceneLoad)
            m_currentScenePath = scenePath;
        m_restoreEditAfterSceneLoad = false;
        SelectFirstEntity(m_gameplayState->GetWorld());
    }

    void EditorState::OpenAssetFile(const std::filesystem::path& path) const
    {
#if defined(_WIN32)
        const std::wstring widePath = path.wstring();
        HINSTANCE result = ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        if (reinterpret_cast<intptr_t>(result) <= 32)
            UWU_ENGINE_WARN("[Editor] Failed to open asset '{}'", path.string());
#else
        UWU_ENGINE_WARN("[Editor] Opening assets is not implemented on this platform: '{}'", path.string());
#endif
    }

    void EditorState::SelectFirstEntity(World& world)
    {
        const std::vector<EntityId> entities = world.GetEntities();
        m_selectedEntity = entities.empty() ? kInvalidEntity : entities.front();
    }

    std::filesystem::path EditorState::ResolveAssetsRoot(const std::string& scenePath) const
    {
        if (scenePath.empty())
            return "Assets";

        std::filesystem::path path(scenePath);
        path = path.lexically_normal();

        std::filesystem::path result;
        for (const auto& part : path)
        {
            result /= part;
            if (part == "Assets")
                return result;
        }

        return path.parent_path().parent_path();
    }
}
