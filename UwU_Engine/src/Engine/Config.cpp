#include "uwupch.h"
#include "Config.h"

#include <nlohmann/json.hpp>

namespace UwU_Engine
{
    const JsonValue& JsonValue::GetNull()
    {
        static const JsonValue s_null;
        return s_null;
    }

    bool        JsonValue::AsBool(bool        def) const { return m_type == Type::Bool ? m_bool : def; }
    float       JsonValue::AsFloat(float       def) const { return m_type == Type::Number ? static_cast<float>(m_num) : def; }
    int         JsonValue::AsInt(int         def) const { return m_type == Type::Number ? static_cast<int>(m_num) : def; }
    std::string JsonValue::AsString(const std::string& def) const { return m_type == Type::String ? m_str : def; }

    const JsonValue& JsonValue::operator[](size_t idx) const
    {
        return (m_type == Type::Array && idx < m_arr.size()) ? m_arr[idx] : GetNull();
    }
    const JsonValue& JsonValue::operator[](const std::string& key) const
    {
        if (m_type == Type::Object) { auto it = m_obj.find(key); if (it != m_obj.end()) return it->second; }
        return GetNull();
    }
    size_t JsonValue::Size() const
    {
        if (m_type == Type::Array)  return m_arr.size();
        if (m_type == Type::Object) return m_obj.size();
        return 0;
    }

    class JsonParser
    {
    public:
        explicit JsonParser(const std::string& src) : m_src(src) {}

        JsonValue Parse()
        {
            return Convert(nlohmann::json::parse(m_src));
        }

    private:
        JsonValue Convert(const nlohmann::json& value) const
        {
            if (value.is_null())
                return JsonValue{};

            if (value.is_boolean())
                return JsonValue(value.get<bool>());

            if (value.is_number())
                return JsonValue(value.get<double>());

            if (value.is_string())
                return JsonValue(value.get<std::string>());

            if (value.is_array())
            {
                JsonValue result;
                result.m_type = JsonValue::Type::Array;
                result.m_arr.reserve(value.size());
                for (const auto& item : value)
                    result.m_arr.push_back(Convert(item));
                return result;
            }

            if (value.is_object())
            {
                JsonValue result;
                result.m_type = JsonValue::Type::Object;
                for (const auto& [key, item] : value.items())
                    result.m_obj[key] = Convert(item);
                return result;
            }

            return JsonValue{};
        }

        const std::string& m_src;
    };

    bool Config::Load(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open()) { UWU_ENGINE_WARN("[Config] Cannot open '{}'", filePath); return false; }
        std::ostringstream ss; ss << file.rdbuf();
        try {
            m_root = JsonParser(ss.str()).Parse();
            m_loaded = true;
            UWU_ENGINE_INFO("[Config] Loaded '{}'", filePath);
            return true;
        }
        catch (const std::exception& e) {
            UWU_ENGINE_ERROR("[Config] Parse error in '{}': {}", filePath, e.what());
            return false;
        }
    }

    const JsonValue& Config::Get(const std::string& dotPath) const
    {
        const JsonValue* cur = &m_root;
        std::string tok;
        for (size_t i = 0; i <= dotPath.size(); ++i) {
            char c = (i < dotPath.size()) ? dotPath[i] : '\0';
            if (c == '.' || c == '\0') {
                if (tok.empty()) return JsonValue::GetNull();
                cur = &(*cur)[tok];
                if (cur->IsNull()) return *cur;
                tok.clear();
            }
            else tok += c;
        }
        return *cur;
    }

    float       Config::GetFloat(const std::string& p, float       d) const { return Get(p).AsFloat(d); }
    int         Config::GetInt(const std::string& p, int         d) const { return Get(p).AsInt(d); }
    std::string Config::GetString(const std::string& p, const std::string& d) const { return Get(p).AsString(d); }

    std::array<float, 3> Config::GetColor3(const std::string& path, std::array<float, 3> def) const
    {
        const auto& v = Get(path);
        if (!v.IsArray() || v.Size() < 3) return def;
        return { v[0].AsFloat(def[0]), v[1].AsFloat(def[1]), v[2].AsFloat(def[2]) };
    }

}
