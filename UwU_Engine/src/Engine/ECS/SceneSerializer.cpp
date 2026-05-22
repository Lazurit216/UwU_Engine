#include "uwupch.h"
#include "SceneSerializer.h"

#include "Engine/ECS/Components.h"

#include <nlohmann/json.hpp>

namespace UwU_Engine
{
    namespace
    {
        using Json = nlohmann::json;

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

        const Json* FindMember(const Json& object, const char* key)
        {
            if (!object.is_object())
                return nullptr;

            const auto it = object.find(key);
            return it == object.end() ? nullptr : &(*it);
        }

        float ReadFloat(const Json& object, const char* key, float fallback)
        {
            const Json* value = FindMember(object, key);
            return value && value->is_number() ? value->get<float>() : fallback;
        }

        int ReadInt(const Json& object, const char* key, int fallback)
        {
            const Json* value = FindMember(object, key);
            return value && value->is_number() ? static_cast<int>(value->get<double>()) : fallback;
        }

        bool ReadBool(const Json& object, const char* key, bool fallback)
        {
            const Json* value = FindMember(object, key);
            return value && value->is_boolean() ? value->get<bool>() : fallback;
        }

        std::string ReadString(const Json& object, const char* key, const std::string& fallback = "")
        {
            const Json* value = FindMember(object, key);
            return value && value->is_string() ? value->get<std::string>() : fallback;
        }

        float ReadArrayFloat(const Json& array, size_t index, float fallback)
        {
            if (!array.is_array() || index >= array.size() || !array[index].is_number())
                return fallback;

            return array[index].get<float>();
        }

        Color4 ReadColor(const Json& value, Color4 fallback = {})
        {
            if (!value.is_array() || value.size() < 4)
                return fallback;

            return Color4{
                ReadArrayFloat(value, 0, fallback.r),
                ReadArrayFloat(value, 1, fallback.g),
                ReadArrayFloat(value, 2, fallback.b),
                ReadArrayFloat(value, 3, fallback.a)
            };
        }

        Json WriteTransform(const TransformComponent& transform)
        {
            return Json{
                { "x", transform.x },
                { "y", transform.y },
                { "z", transform.z },
                { "rotationX", transform.rotationX },
                { "rotationY", transform.rotationY },
                { "rotationZ", transform.rotationZ },
                { "scaleX", transform.scaleX },
                { "scaleY", transform.scaleY },
                { "scaleZ", transform.scaleZ }
            };
        }

        TransformComponent ReadTransform(const Json& value)
        {
            TransformComponent transform;
            transform.x = ReadFloat(value, "x", 0.0f);
            transform.y = ReadFloat(value, "y", 0.0f);
            transform.z = ReadFloat(value, "z", 0.0f);
            transform.rotationX = ReadFloat(value, "rotationX", 0.0f);
            transform.rotationY = ReadFloat(value, "rotationY", 0.0f);
            transform.rotationZ = ReadFloat(value, "rotationZ", 0.0f);
            transform.scaleX = ReadFloat(value, "scaleX", 1.0f);
            transform.scaleY = ReadFloat(value, "scaleY", 1.0f);
            transform.scaleZ = ReadFloat(value, "scaleZ", 1.0f);
            return transform;
        }

        std::string ResolveSceneAssetPath(const std::string& scenePath, const std::string& assetPath)
        {
            if (assetPath.empty())
                return {};

            std::filesystem::path path(assetPath);
            if (path.is_absolute() || std::filesystem::exists(path))
                return path.string();

            std::filesystem::path sceneFile(scenePath);
            sceneFile = sceneFile.lexically_normal();

            std::filesystem::path sourcePrefix;
            for (const auto& part : sceneFile)
            {
                if (part == "Assets")
                    break;

                sourcePrefix /= part;
            }

            if (!sourcePrefix.empty())
            {
                std::filesystem::path sourceAssetPath = sourcePrefix / path;
                if (std::filesystem::exists(sourceAssetPath))
                    return sourceAssetPath.string();
            }

            std::filesystem::path sceneRelativePath = sceneFile.parent_path() / path;
            if (std::filesystem::exists(sceneRelativePath))
                return sceneRelativePath.string();

            return path.string();
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

        Json root;
        root["version"] = 1;
        root["entities"] = Json::array();

        const auto entities = world.GetEntities();
        for (EntityId entity : entities)
        {
            Json node;
            node["id"] = entity;

            if (const auto* tag = world.GetComponent<TagComponent>(entity))
                node["tag"] = Json{ { "name", tag->name } };

            if (const auto* transform = world.GetComponent<TransformComponent>(entity))
                node["transform"] = WriteTransform(*transform);

            if (const auto* mesh = world.GetComponent<MeshRendererComponent>(entity))
            {
                const Color4 color = mesh->material.baseColor;
                node["meshRenderer"] = Json{
                    { "primitive", PrimitiveToString(mesh->mesh.primitive) },
                    { "meshPath", mesh->meshResourcePath.empty() ? mesh->mesh.sourcePath : mesh->meshResourcePath },
                    { "materialPath", mesh->materialResourcePath },
                    { "shaderPath", NarrowPath(mesh->material.shaderPath) },
                    { "texturePath", mesh->material.texturePath },
                    { "color", Json::array({ color.r, color.g, color.b, color.a }) }
                };
            }

            if (const auto* hierarchy = world.GetComponent<HierarchyComponent>(entity))
            {
                node["hierarchy"] = Json{
                    { "parent", hierarchy->parent },
                    { "children", hierarchy->children }
                };
            }

            if (const auto* camera = world.GetComponent<CameraComponent>(entity))
            {
                node["camera"] = Json{
                    { "primary", camera->primary },
                    { "zoom", camera->zoom },
                    { "viewHalfWidth", camera->viewHalfWidth },
                    { "viewHalfHeight", camera->viewHalfHeight },
                    { "fovYRadians", camera->fovYRadians },
                    { "nearPlane", camera->nearPlane },
                    { "farPlane", camera->farPlane },
                    { "moveSpeed", camera->moveSpeed },
                    { "zoomSpeed", camera->zoomSpeed }
                };
            }

            root["entities"].push_back(std::move(node));
        }

        std::ofstream out(filePath, std::ios::trunc);
        if (!out.is_open())
        {
            UWU_ENGINE_WARN("[SceneSerializer] Cannot open '{}' for writing", filePath);
            return false;
        }

        out << root.dump(2) << '\n';
        UWU_ENGINE_INFO("[SceneSerializer] Saved scene to '{}'", filePath);
        return true;
    }

    bool SceneSerializer::Load(World& world, const std::string& filePath, const std::wstring& fallbackShaderPath) const
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            UWU_ENGINE_WARN("[SceneSerializer] Cannot open '{}'", filePath);
            return false;
        }

        Json root;
        try
        {
            root = Json::parse(file);
        }
        catch (const std::exception& e)
        {
            UWU_ENGINE_ERROR("[SceneSerializer] Parse error in '{}': {}", filePath, e.what());
            return false;
        }

        const Json* entities = FindMember(root, "entities");
        if (!entities || !entities->is_array())
        {
            UWU_ENGINE_WARN("[SceneSerializer] '{}' has no entities array", filePath);
            return false;
        }

        world.Clear();

        std::unordered_map<int, EntityId> remap;
        for (size_t i = 0; i < entities->size(); ++i)
        {
            const Json& node = (*entities)[i];
            const int oldId = ReadInt(node, "id", static_cast<int>(i + 1));
            remap[oldId] = world.CreateEntity();
        }

        for (size_t i = 0; i < entities->size(); ++i)
        {
            const Json& node = (*entities)[i];
            const int oldId = ReadInt(node, "id", static_cast<int>(i + 1));
            const EntityId entity = remap[oldId];

            const Json* tag = FindMember(node, "tag");
            if (tag && tag->is_object())
                world.AddComponent<TagComponent>(entity, TagComponent{ ReadString(*tag, "name", "Entity") });

            const Json* transform = FindMember(node, "transform");
            if (transform && transform->is_object())
                world.AddComponent<TransformComponent>(entity, ReadTransform(*transform));

            const Json* meshRenderer = FindMember(node, "meshRenderer");
            if (meshRenderer && meshRenderer->is_object())
            {
                const Json* colorValue = FindMember(*meshRenderer, "color");
                const Color4 color = colorValue ? ReadColor(*colorValue) : Color4{};
                const PrimitiveType primitive = PrimitiveFromString(
                    ReadString(*meshRenderer, "primitive", "Triangle"));

                MeshData mesh;
                if (primitive == PrimitiveType::Triangle)
                    mesh = MeshFactory::CreateTriangle(1.0f, color);
                else
                    mesh.primitive = primitive;

                const std::string meshPath = ResolveSceneAssetPath(
                    filePath,
                    ReadString(*meshRenderer, "meshPath"));
                const std::string texturePath = ResolveSceneAssetPath(
                    filePath,
                    ReadString(*meshRenderer, "texturePath"));
                const std::string materialPath = ResolveSceneAssetPath(
                    filePath,
                    ReadString(*meshRenderer, "materialPath"));

                mesh.sourcePath = meshPath;

                MaterialDesc material;
                material.baseColor = color;
                material.texturePath = texturePath;

                const std::string shaderPath = ReadString(*meshRenderer, "shaderPath");
                material.shaderPath = shaderPath.empty() ? fallbackShaderPath : WidenPath(shaderPath);

                auto& component = world.AddComponent<MeshRendererComponent>(
                    entity,
                    MeshRendererComponent{ std::move(mesh), std::move(material) });
                component.meshResourcePath = meshPath;
                component.materialResourcePath = materialPath;
            }

            const Json* camera = FindMember(node, "camera");
            if (camera && camera->is_object())
            {
                CameraComponent component;
                component.primary = ReadBool(*camera, "primary", true);
                component.zoom = std::clamp(ReadFloat(*camera, "zoom", 1.0f), 0.25f, 4.0f);
                component.viewHalfWidth = ReadFloat(*camera, "viewHalfWidth", 1.8f);
                component.viewHalfHeight = ReadFloat(*camera, "viewHalfHeight", 1.0f);
                component.fovYRadians = ReadFloat(*camera, "fovYRadians", 1.04719755f);
                component.nearPlane = ReadFloat(*camera, "nearPlane", 0.01f);
                component.farPlane = ReadFloat(*camera, "farPlane", 100.0f);
                component.moveSpeed = ReadFloat(*camera, "moveSpeed", 0.8f);
                component.zoomSpeed = ReadFloat(*camera, "zoomSpeed", 1.0f);
                world.AddComponent<CameraComponent>(entity, component);
            }
        }

        for (size_t i = 0; i < entities->size(); ++i)
        {
            const Json& node = (*entities)[i];
            const Json* hierarchy = FindMember(node, "hierarchy");
            if (!hierarchy || !hierarchy->is_object())
                continue;

            const int oldId = ReadInt(node, "id", static_cast<int>(i + 1));
            const EntityId entity = remap[oldId];
            auto& component = world.AddComponent<HierarchyComponent>(entity);

            const int oldParent = ReadInt(*hierarchy, "parent", 0);
            auto parentIt = remap.find(oldParent);
            component.parent = parentIt != remap.end() ? parentIt->second : kInvalidEntity;
        }

        RebuildHierarchyChildren(world);
        UWU_ENGINE_INFO("[SceneSerializer] Loaded scene '{}' ({} entities)", filePath, world.EntityCount());
        return true;
    }
}
