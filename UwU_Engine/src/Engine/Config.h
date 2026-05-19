#pragma once
#include "Core.h"

namespace UwU_Engine
{
    class JsonValue
    {
    public:
        enum class Type { Null, Bool, Number, String, Array, Object };

        JsonValue() : m_type(Type::Null) {}
        explicit JsonValue(bool b) : m_type(Type::Bool), m_bool(b) {}
        explicit JsonValue(double n) : m_type(Type::Number), m_num(n) {}
        explicit JsonValue(std::string s) : m_type(Type::String), m_str(std::move(s)) {}

        bool IsNull()   const { return m_type == Type::Null; }
        bool IsArray()  const { return m_type == Type::Array; }
        bool IsObject() const { return m_type == Type::Object; }

        bool AsBool(bool def = false) const;
        float AsFloat(float def = 0.f) const;
        int AsInt(int def = 0) const;
        std::string AsString(const std::string& def = "") const;

        const JsonValue& operator[](size_t idx) const;
        const JsonValue& operator[](const std::string& key) const;
        size_t Size() const;

        static const JsonValue& GetNull();
    private:
        Type m_type = Type::Null;
        bool m_bool = false;
        double m_num = 0.0;
        std::string m_str;
        std::vector<JsonValue> m_arr;
        std::unordered_map<std::string, JsonValue> m_obj;

        //static const JsonValue s_null;
        friend class JsonParser;
    };


    class UWU_API Config
    {
    public:
        bool Load(const std::string& filePath);

        // Dot-notation access - returns null for missing paths.
        const JsonValue& Get(const std::string& dotPath) const;

        float GetFloat(const std::string& path, float def = 0.f) const;
        int GetInt(const std::string& path, int def = 0)   const;
        std::string GetString(const std::string& path, const std::string& def = "") const;

        // Reads a JSON array [r, g, b] into 3 floats.
        std::array<float, 3> GetColor3(const std::string& path, std::array<float, 3> def = {}) const;

        bool IsLoaded() const { return m_loaded; }

    private:
        JsonValue m_root;
        bool m_loaded = false;
    };

} 
