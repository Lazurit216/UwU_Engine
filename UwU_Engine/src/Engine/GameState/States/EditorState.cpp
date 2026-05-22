#include "uwupch.h"
#include "EditorState.h"

#include "Engine/Logger.h"

#include "imgui.h"
#include "imgui_internal.h"

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

    }

    EditorState::EditorState(std::shared_ptr<GameplayState> gameplayState, std::string scenePath)
        : m_gameplayState(std::move(gameplayState))
        , m_assetsRoot(ResolveAssetsRoot(scenePath))
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

        m_gameplayState->SetPhysicsDebugVisible(m_debugDrawEnabled);

        StateTransition transition = StateTransition::None();
        if (m_simulationEnabled)
            transition = m_gameplayState->Update(ctx, dt);

        if (m_selectedEntity != kInvalidEntity && !m_gameplayState->GetWorld().IsAlive(m_selectedEntity))
            SelectFirstEntity(m_gameplayState->GetWorld());

        return transition;
    }

    void EditorState::Render(const StateContext& ctx)
    {
        if (m_gameplayState)
            m_gameplayState->Render(ctx);

        if (ImGui::GetCurrentContext())
            DrawEditorUi();
    }

    void EditorState::DrawEditorUi()
    {
        DrawDockspace();
        DrawToolbar();

        if (!m_gameplayState)
            return;

        World& world = m_gameplayState->GetWorld();
        DrawHierarchy(world);
        DrawInspector(world);
        DrawAssets();
        DrawConsole();
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

            ImGui::DockBuilderDockWindow("Hierarchy", leftId);
            ImGui::DockBuilderDockWindow("Inspector", rightId);
            ImGui::DockBuilderDockWindow("Assets", bottomId);
            ImGui::DockBuilderDockWindow("Console", bottomId);
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
            if (ImGui::MenuItem("Refresh Assets"))
                RefreshAssets();
            ImGui::EndMenu();
        }

        ImGui::Separator();
        if (ImGui::Button(m_simulationEnabled ? "Pause" : "Play"))
            m_simulationEnabled = !m_simulationEnabled;
        ImGui::SameLine();
        ImGui::Checkbox("Debug Draw", &m_debugDrawEnabled);
        ImGui::SameLine();
        ImGui::Text("Selected: %u", m_selectedEntity);

        ImGui::EndMainMenuBar();
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
        const auto* tag = world.GetComponent<TagComponent>(entity);
        const auto* hierarchy = world.GetComponent<HierarchyComponent>(entity);
        const std::string label = std::format("{}##entity_{}",
            tag ? tag->name : std::format("Entity {}", entity),
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
                ImGui::ColorEdit4("Base Color", &mesh->material.baseColor.r);
                if (ImGui::Button("Remove Mesh Renderer"))
                    world.RemoveComponent<MeshRendererComponent>(m_selectedEntity);
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
            }
        }

        if (auto* body = world.GetComponent<RigidbodyComponent>(m_selectedEntity))
        {
            if (ImGui::CollapsingHeader("Rigidbody", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Velocity", &body->velocity.x, 0.01f);
                ImGui::DragFloat("Mass", &body->mass, 0.05f, 0.0f, 100.0f);
                ImGui::Checkbox("Use Gravity", &body->useGravity);
                ImGui::Checkbox("Static", &body->isStatic);
                ImGui::DragFloat("Damping", &body->linearDamping, 0.01f, 0.0f, 10.0f);
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
            }
        }

        DrawComponentButtons(world, m_selectedEntity);

        ImGui::Separator();
        if (ImGui::Button("Delete Entity"))
        {
            world.DestroyEntity(m_selectedEntity);
            SelectFirstEntity(world);
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
            const bool hasRigidbody = world.HasComponent<RigidbodyComponent>(entity);
            const bool hasCollider = world.HasComponent<ColliderComponent>(entity);
            const bool hasCamera = world.HasComponent<CameraComponent>(entity);
            const bool hasController = world.HasComponent<PlayerControllerComponent>(entity);
            const bool hasHierarchy = world.HasComponent<HierarchyComponent>(entity);

            if (ImGui::MenuItem(hasTransform ? "Transform already exists" : "Add Transform", nullptr, false, !hasTransform))
                world.AddComponent<TransformComponent>(entity);
            if (ImGui::MenuItem(hasRigidbody ? "Rigidbody already exists" : "Add Rigidbody", nullptr, false, !hasRigidbody))
                world.AddComponent<RigidbodyComponent>(entity);
            if (ImGui::MenuItem(hasCollider ? "Collider already exists" : "Add Collider", nullptr, false, !hasCollider))
                world.AddComponent<ColliderComponent>(entity);
            if (ImGui::MenuItem(hasCamera ? "Camera already exists" : "Add Camera", nullptr, false, !hasCamera))
                world.AddComponent<CameraComponent>(entity);
            if (ImGui::MenuItem(hasController ? "Player Controller already exists" : "Add Player Controller", nullptr, false, !hasController))
                world.AddComponent<PlayerControllerComponent>(entity);
            if (ImGui::MenuItem(hasHierarchy ? "Hierarchy already exists" : "Add Hierarchy", nullptr, false, !hasHierarchy))
                world.AddComponent<HierarchyComponent>(entity);
            ImGui::EndPopup();
        }

        if (world.HasComponent<RigidbodyComponent>(entity))
        {
            ImGui::SameLine();
            if (ImGui::Button("Remove Rigidbody"))
                world.RemoveComponent<RigidbodyComponent>(entity);
        }
        if (world.HasComponent<ColliderComponent>(entity))
        {
            ImGui::SameLine();
            if (ImGui::Button("Remove Collider"))
                world.RemoveComponent<ColliderComponent>(entity);
        }
    }

    void EditorState::DrawAssets()
    {
        ImGui::Begin("Assets");

        if (ImGui::Button("Refresh"))
            RefreshAssets();
        ImGui::SameLine();
        ImGui::Text("%s", m_assetsRoot.string().c_str());
        ImGui::Separator();

        for (const auto& path : m_assetEntries)
            ImGui::Selectable(path.lexically_relative(m_assetsRoot).string().c_str());

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
