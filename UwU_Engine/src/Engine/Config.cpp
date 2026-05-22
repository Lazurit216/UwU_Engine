#include "uwupch.h"
#include "Config.h"

#include <nlohmann/json.hpp>

namespace UwU_Engine
{
    struct Config::Storage
    {
        nlohmann::json root;
    };

    namespace
    {
        const nlohmann::json* FindValue(const nlohmann::json& root, const std::string& dotPath)
        {
            const nlohmann::json* current = &root;
            size_t tokenStart = 0;

            while (tokenStart <= dotPath.size())
            {
                const size_t tokenEnd = dotPath.find('.', tokenStart);
                const std::string key = dotPath.substr(
                    tokenStart,
                    tokenEnd == std::string::npos ? std::string::npos : tokenEnd - tokenStart);

                if (key.empty() || !current->is_object())
                    return nullptr;

                const auto it = current->find(key);
                if (it == current->end())
                    return nullptr;

                current = &(*it);

                if (tokenEnd == std::string::npos)
                    break;

                tokenStart = tokenEnd + 1;
            }

            return current;
        }
    }

    Config::Config()
        : m_storage(std::make_unique<Storage>())
    {
    }

    Config::~Config() = default;
    Config::Config(Config&&) noexcept = default;
    Config& Config::operator=(Config&&) noexcept = default;

    bool Config::Load(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            UWU_ENGINE_WARN("[Config] Cannot open '{}'", filePath);
            return false;
        }

        try
        {
            m_storage->root = nlohmann::json::parse(file);
            m_loaded = true;
            UWU_ENGINE_INFO("[Config] Loaded '{}'", filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            UWU_ENGINE_ERROR("[Config] Parse error in '{}': {}", filePath, e.what());
            m_loaded = false;
            return false;
        }
    }

    float Config::GetFloat(const std::string& path, float def) const
    {
        const nlohmann::json* value = FindValue(m_storage->root, path);
        return value && value->is_number() ? value->get<float>() : def;
    }

    int Config::GetInt(const std::string& path, int def) const
    {
        const nlohmann::json* value = FindValue(m_storage->root, path);
        return value && value->is_number() ? static_cast<int>(value->get<double>()) : def;
    }

    std::string Config::GetString(const std::string& path, const std::string& def) const
    {
        const nlohmann::json* value = FindValue(m_storage->root, path);
        return value && value->is_string() ? value->get<std::string>() : def;
    }

    std::array<float, 3> Config::GetColor3(const std::string& path, std::array<float, 3> def) const
    {
        const nlohmann::json* value = FindValue(m_storage->root, path);
        if (!value || !value->is_array() || value->size() < 3)
            return def;

        std::array<float, 3> color = def;
        for (size_t i = 0; i < color.size(); ++i)
        {
            if ((*value)[i].is_number())
                color[i] = (*value)[i].get<float>();
        }

        return color;
    }
}
