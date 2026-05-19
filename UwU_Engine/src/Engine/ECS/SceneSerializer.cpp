#include "uwupch.h"
#include "SceneSerializer.h"

#include "Engine/Config.h"
#include "Engine/ECS/Components.h"

namespace UwU_Engine
{
    namespace
    {
        std::string EscapeJson(const std::string& value)
        {
            std::ostringstream out;
            for (char ch : value)
            {
                switch (ch)
                {
                case '"':  out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\n': out << "\\n";  break;
                case '\r': out << "\\r";  break;
                case '\t': out << "\\t";  break;
                default:   out << ch;      break;
                }
            }
            return out.str();
        }

        std::string NarrowPath(const std::wstring& value)
        {
            std::string result;
            result.reserve(value.size());
            for (wchar_t ch : value)
                result.push_back(ch >= 0 && ch <= 127 ? static_cast<char>(ch) : '?');
            return result;
        }

        std::wstring WidenPath(const std::string& value)
        {
            std::wstring result;
            result.reserve(value.size());
            for (char ch : value)
                result.push_back(static_cast<unsigned char>(ch));
            return result;
        }

        const char* PrimitiveToString(PrimitiveType primitive)
        {
            switch (primitive)
            {
            case PrimitiveType::Line:       return "Line";
            case PrimitiveType::Triangle:   return "Triangle";
            case PrimitiveType::Quad:       return "Quad";
            case PrimitiveType::Cube:       return "Cube";
            case PrimitiveType::CustomMesh: return "CustomMesh";
            default:                        return "Triangle";
            }
        }

        PrimitiveType PrimitiveFromString(const std::string& value)
        {
            if (value == "Line") return PrimitiveType::Line;
            if (value == "Quad") return PrimitiveType::Quad;
            if (value == "Cube") return PrimitiveType::Cube;
            if (value == "CustomMesh") return PrimitiveType::CustomMesh;
            return PrimitiveType::Triangle;
        }

        Color4 ReadColor(const JsonValue& value, Color4 fallback = {})
        {
            if (!value.IsArray() || value.Size() < 4)
                return fallback;

            return Color4{
                value[0].AsFloat(fallback.r),
                value[1].AsFloat(fallback.g),
                value[2].AsFloat(fallback.b),
                value[3].AsFloat(fallback.a)
            };
        }

        void WriteTransform(std::ostream& out, const TransformComponent& transform)
        {
            out << "{ "
                << "\"x\": " << transform.x << ", "
                << "\"y\": " << transform.y << ", "
                << "\"z\": " << transform.z << ", "
                << "\"rotationZ\": " << transform.rotationZ << ", "
                << "\"scaleX\": " << transform.scaleX << ", "
                << "\"scaleY\": " << transform.scaleY << ", "
                << "\"scaleZ\": " << transform.scaleZ
                << " }";
        }

        TransformComponent ReadTransform(const JsonValue& value)
        {
            TransformComponent transform;
            transform.x = value["x"].AsFloat(0.0f);
            transform.y = value["y"].AsFloat(0.0f);
            transform.z = value["z"].AsFloat(0.0f);
            transform.rotationZ = value["rotationZ"].AsFloat(0.0f);
            transform.scaleX = value["scaleX"].AsFloat(1.0f);
            transform.scaleY = value["scaleY"].AsFloat(1.0f);
            transform.scaleZ = value["scaleZ"].AsFloat(1.0f);
            return transform;
        }

        void RebuildHierarchyChildren(World& world)
        {
            std::vector<std::pair<EntityId, EntityId>> links;

            world.ForEach<HierarchyComponent>(
                [&links, &world](EntityId entity, HierarchyComponent& hierarchy)
                {
                    hierarchy.children.clear();

                    if (hierarchy.parent == kInvalidEntity || !world.IsAlive(hierarchy.parent))
                        return;

                    links.emplace_back(hierarchy.parent, entity);
                });

            for (const auto& [parent, child] : links)
            {
                auto* parentHierarchy = world.GetComponent<HierarchyComponent>(parent);
                if (!parentHierarchy)
                    parentHierarchy = &world.AddComponent<HierarchyComponent>(parent);

                parentHierarchy->children.push_back(child);
            }
        }
    }

    bool SceneSerializer::Save(World& world, const std::string& filePath) const
    {
        const std::filesystem::path path(filePath);
        if (path.has_parent_path())
            std::filesystem::create_directories(path.parent_path());

        std::ofstream out(filePath, std::ios::trunc);
        if (!out.is_open())
        {
            UWU_ENGINE_WARN("[SceneSerializer] Cannot open '{}' for writing", filePath);
            return false;
        }

        out << std::fixed << std::setprecision(4);
        out << "{\n";
        out << "  \"version\": 1,\n";
        out << "  \"entities\": [\n";

        const auto entities = world.GetEntities();
        bool firstEntity = true;
        for (EntityId entity : entities)
        {
            if (!firstEntity)
                out << ",\n";
            firstEntity = false;

            out << "    {\n";
            out << "      \"id\": " << entity;

            if (const auto* tag = world.GetComponent<TagComponent>(entity))
            {
                out << ",\n      \"tag\": { \"name\": \""
                    << EscapeJson(tag->name) << "\" }";
            }

            if (const auto* transform = world.GetComponent<TransformComponent>(entity))
            {
                out << ",\n      \"transform\": ";
                WriteTransform(out, *transform);
            }

            if (const auto* mesh = world.GetComponent<MeshRendererComponent>(entity))
            {
                const Color4 color = mesh->material.baseColor;
                out << ",\n      \"meshRenderer\": {\n";
                out << "        \"primitive\": \"" << PrimitiveToString(mesh->mesh.primitive) << "\",\n";
                out << "        \"meshPath\": \"" << EscapeJson(mesh->meshResourcePath.empty() ? mesh->mesh.sourcePath : mesh->meshResourcePath) << "\",\n";
                out << "        \"shaderPath\": \"" << EscapeJson(NarrowPath(mesh->material.shaderPath)) << "\",\n";
                out << "        \"texturePath\": \"" << EscapeJson(mesh->material.texturePath) << "\",\n";
                out << "        \"color\": [ "
                    << color.r << ", " << color.g << ", " << color.b << ", " << color.a << " ]\n";
                out << "      }";
            }

            if (const auto* hierarchy = world.GetComponent<HierarchyComponent>(entity))
            {
                out << ",\n      \"hierarchy\": {\n";
                out << "        \"parent\": " << hierarchy->parent << ",\n";
                out << "        \"children\": [ ";
                for (size_t i = 0; i < hierarchy->children.size(); ++i)
                {
                    if (i > 0)
                        out << ", ";
                    out << hierarchy->children[i];
                }
                out << " ]\n";
                out << "      }";
            }

            if (const auto* camera = world.GetComponent<CameraComponent>(entity))
            {
                out << ",\n      \"camera\": {\n";
                out << "        \"primary\": " << (camera->primary ? "true" : "false") << ",\n";
                out << "        \"zoom\": " << camera->zoom << ",\n";
                out << "        \"viewHalfWidth\": " << camera->viewHalfWidth << ",\n";
                out << "        \"viewHalfHeight\": " << camera->viewHalfHeight << ",\n";
                out << "        \"moveSpeed\": " << camera->moveSpeed << ",\n";
                out << "        \"zoomSpeed\": " << camera->zoomSpeed << "\n";
                out << "      }";
            }

            out << "\n    }";
        }

        out << "\n  ]\n";
        out << "}\n";

        UWU_ENGINE_INFO("[SceneSerializer] Saved scene to '{}'", filePath);
        return true;
    }

    bool SceneSerializer::Load(World& world, const std::string& filePath, const std::wstring& fallbackShaderPath) const
    {
        Config config;
        if (!config.Load(filePath))
            return false;

        const JsonValue& entities = config.Get("entities");
        if (!entities.IsArray())
        {
            UWU_ENGINE_WARN("[SceneSerializer] '{}' has no entities array", filePath);
            return false;
        }

        world.Clear();

        std::unordered_map<int, EntityId> remap;
        for (size_t i = 0; i < entities.Size(); ++i)
        {
            const JsonValue& node = entities[i];
            const int oldId = node["id"].AsInt(static_cast<int>(i + 1));
            remap[oldId] = world.CreateEntity();
        }

        for (size_t i = 0; i < entities.Size(); ++i)
        {
            const JsonValue& node = entities[i];
            const int oldId = node["id"].AsInt(static_cast<int>(i + 1));
            const EntityId entity = remap[oldId];

            const JsonValue& tag = node["tag"];
            if (tag.IsObject())
                world.AddComponent<TagComponent>(entity, TagComponent{ tag["name"].AsString("Entity") });

            const JsonValue& transform = node["transform"];
            if (transform.IsObject())
                world.AddComponent<TransformComponent>(entity, ReadTransform(transform));

            const JsonValue& meshRenderer = node["meshRenderer"];
            if (meshRenderer.IsObject())
            {
                const Color4 color = ReadColor(meshRenderer["color"]);
                const PrimitiveType primitive = PrimitiveFromString(meshRenderer["primitive"].AsString("Triangle"));

                MeshData mesh;
                if (primitive == PrimitiveType::Triangle)
                    mesh = MeshFactory::CreateTriangle(1.0f, color);
                else
                    mesh.primitive = primitive;

                mesh.sourcePath = meshRenderer["meshPath"].AsString("");

                MaterialDesc material;
                material.baseColor = color;
                material.texturePath = meshRenderer["texturePath"].AsString("");

                const std::string shaderPath = meshRenderer["shaderPath"].AsString("");
                material.shaderPath = shaderPath.empty() ? fallbackShaderPath : WidenPath(shaderPath);

                auto& component = world.AddComponent<MeshRendererComponent>(
                    entity,
                    MeshRendererComponent{ std::move(mesh), std::move(material) });
                component.meshResourcePath = meshRenderer["meshPath"].AsString("");
            }

            const JsonValue& camera = node["camera"];
            if (camera.IsObject())
            {
                CameraComponent component;
                component.primary = camera["primary"].AsBool(true);
                component.zoom = std::clamp(camera["zoom"].AsFloat(1.0f), 0.25f, 4.0f);
                component.viewHalfWidth = camera["viewHalfWidth"].AsFloat(1.8f);
                component.viewHalfHeight = camera["viewHalfHeight"].AsFloat(1.0f);
                component.moveSpeed = camera["moveSpeed"].AsFloat(0.8f);
                component.zoomSpeed = camera["zoomSpeed"].AsFloat(1.0f);
                world.AddComponent<CameraComponent>(entity, component);
            }
        }

        for (size_t i = 0; i < entities.Size(); ++i)
        {
            const JsonValue& node = entities[i];
            const JsonValue& hierarchy = node["hierarchy"];
            if (!hierarchy.IsObject())
                continue;

            const int oldId = node["id"].AsInt(static_cast<int>(i + 1));
            const EntityId entity = remap[oldId];
            auto& component = world.AddComponent<HierarchyComponent>(entity);

            const int oldParent = hierarchy["parent"].AsInt(0);
            auto parentIt = remap.find(oldParent);
            component.parent = parentIt != remap.end() ? parentIt->second : kInvalidEntity;
        }

        RebuildHierarchyChildren(world);
        UWU_ENGINE_INFO("[SceneSerializer] Loaded scene '{}' ({} entities)", filePath, world.EntityCount());
        return true;
    }
}
