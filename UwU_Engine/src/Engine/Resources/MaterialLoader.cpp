#include "uwupch.h"
#include "MaterialLoader.h"

#include <nlohmann/json.hpp>

namespace UwU_Engine
{
    namespace
    {
        using Json = nlohmann::json;

        const Json* FindMember(const Json& object, const char* key)
        {
            if (!object.is_object())
                return nullptr;

            const auto it = object.find(key);
            return it == object.end() ? nullptr : &(*it);
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

        std::wstring WidenPath(const std::string& value)
        {
            std::wstring result;
            result.reserve(value.size());
            for (char ch : value)
                result.push_back(static_cast<unsigned char>(ch));
            return result;
        }

        std::string ResolveReferencedPath(const std::string& ownerFilePath, const std::string& referencedPath)
        {
            if (referencedPath.empty())
                return {};

            std::filesystem::path path(referencedPath);
            if (path.is_absolute() || std::filesystem::exists(path))
                return path.lexically_normal().string();

            const std::filesystem::path ownerPath(ownerFilePath);
            return (ownerPath.parent_path() / path).lexically_normal().string();
        }

        bool ReadColor(const Json& object, Color4& color)
        {
            const Json* value = FindMember(object, "color");
            if (!value || !value->is_array() || value->size() < 4)
                return false;

            color = Color4{
                ReadArrayFloat(*value, 0, color.r),
                ReadArrayFloat(*value, 1, color.g),
                ReadArrayFloat(*value, 2, color.b),
                ReadArrayFloat(*value, 3, color.a)
            };
            return true;
        }
    }

    bool MaterialLoader::Load(const std::string& path, MaterialAsset& out, std::string& error)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            error = "cannot open material file";
            return false;
        }

        Json root;
        try
        {
            root = Json::parse(file);
        }
        catch (const std::exception& e)
        {
            error = e.what();
            return false;
        }

        if (!root.is_object())
        {
            error = "material root must be a JSON object";
            return false;
        }

        out = MaterialAsset{};
        out.hasBaseColor = ReadColor(root, out.material.baseColor);

        const Json* shaderPathValue = FindMember(root, "shaderPath");
        if (shaderPathValue && shaderPathValue->is_string())
        {
            const std::string shaderPath = ResolveReferencedPath(path, shaderPathValue->get<std::string>());
            out.material.shaderPath = WidenPath(shaderPath);
            out.hasShaderPath = !shaderPath.empty();
        }

        const Json* texturePathValue = FindMember(root, "texturePath");
        if (texturePathValue && texturePathValue->is_string())
        {
            out.material.texturePath = ResolveReferencedPath(path, texturePathValue->get<std::string>());
            out.hasTexturePath = !out.material.texturePath.empty();
        }

        out.material.vertexEntry = ReadString(root, "vertexEntry", out.material.vertexEntry);
        out.material.pixelEntry = ReadString(root, "pixelEntry", out.material.pixelEntry);
        out.material.vertexProfile = ReadString(root, "vertexProfile", out.material.vertexProfile);
        out.material.pixelProfile = ReadString(root, "pixelProfile", out.material.pixelProfile);

        UWU_ENGINE_INFO("[MaterialLoader] Loaded '{}'", path);
        return true;
    }
}
