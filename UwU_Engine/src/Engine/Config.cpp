#include "uwupch.h"
#include "Config.h"

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
        explicit JsonParser(const std::string& src) : m_src(src), m_pos(0) {}
        JsonValue Parse() { SkipWs(); return ParseValue(); }

    private:
        const std::string& m_src;
        size_t m_pos;

        char  Peek()    const { return m_pos < m_src.size() ? m_src[m_pos] : '\0'; }
        char  Consume() { return m_pos < m_src.size() ? m_src[m_pos++] : '\0'; }
        void  SkipWs() { while (m_pos < m_src.size() && std::isspace((unsigned char)m_src[m_pos])) ++m_pos; }
        void  Expect(char c) { SkipWs(); if (Peek() != c) throw std::runtime_error(std::string("JSON: expected '") + c + "'"); Consume(); }

        JsonValue ParseValue()
        {
            SkipWs();
            char c = Peek();
            if (c == '{') return ParseObject();
            if (c == '[') return ParseArray();
            if (c == '"') return JsonValue(ParseString());
            if (c == 't') { m_pos += 4; return JsonValue(true); }
            if (c == 'f') { m_pos += 5; return JsonValue(false); }
            if (c == 'n') { m_pos += 4; return JsonValue{}; }
            return JsonValue(ParseNumber());
        }

        std::string ParseString()
        {
            Expect('"');
            std::string r;
            while (m_pos < m_src.size()) {
                char ch = Consume();
                if (ch == '"') break;
                if (ch == '\\') {
                    char e = Consume();
                    switch (e) {
                    case '"': r += '"';  break; case '\\': r += '\\'; break;
                    case 'n': r += '\n'; break; case 'r':  r += '\r'; break;
                    case 't': r += '\t'; break; default:   r += e;    break;
                    }
                }
                else r += ch;
            }
            return r;
        }

        double ParseNumber()
        {
            size_t s = m_pos;
            if (Peek() == '-') ++m_pos;
            while (m_pos < m_src.size() && std::isdigit((unsigned char)m_src[m_pos])) ++m_pos;
            if (m_pos < m_src.size() && m_src[m_pos] == '.') {
                ++m_pos;
                while (m_pos < m_src.size() && std::isdigit((unsigned char)m_src[m_pos])) ++m_pos;
            }
            if (m_pos < m_src.size() && (m_src[m_pos] == 'e' || m_src[m_pos] == 'E')) {
                ++m_pos;
                if (m_pos < m_src.size() && (m_src[m_pos] == '+' || m_src[m_pos] == '-')) ++m_pos;
                while (m_pos < m_src.size() && std::isdigit((unsigned char)m_src[m_pos])) ++m_pos;
            }
            return std::stod(m_src.substr(s, m_pos - s));
        }

        JsonValue ParseObject()
        {
            Expect('{');
            JsonValue obj; obj.m_type = JsonValue::Type::Object;
            SkipWs(); if (Peek() == '}') { Consume(); return obj; }
            while (true) {
                SkipWs(); std::string key = ParseString();
                Expect(':'); SkipWs();
                obj.m_obj[key] = ParseValue();
                SkipWs(); if (Peek() == '}') { Consume(); break; } Expect(',');
            }
            return obj;
        }

        JsonValue ParseArray()
        {
            Expect('[');
            JsonValue arr; arr.m_type = JsonValue::Type::Array;
            SkipWs(); if (Peek() == ']') { Consume(); return arr; }
            while (true) {
                SkipWs(); arr.m_arr.push_back(ParseValue());
                SkipWs(); if (Peek() == ']') { Consume(); break; } Expect(',');
            }
            return arr;
        }
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
