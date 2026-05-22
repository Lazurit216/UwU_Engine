#pragma once

#include "Core.h"

#include <array>
#include <memory>
#include <string>

namespace UwU_Engine
{
    class UWU_API Config
    {
    public:
        Config();
        ~Config();

        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
        Config(Config&&) noexcept;
        Config& operator=(Config&&) noexcept;

        bool Load(const std::string& filePath);

        float GetFloat(const std::string& path, float def = 0.f) const;
        int GetInt(const std::string& path, int def = 0) const;
        std::string GetString(const std::string& path, const std::string& def = "") const;

        // Reads a JSON array [r, g, b] into 3 floats.
        std::array<float, 3> GetColor3(const std::string& path, std::array<float, 3> def = {}) const;

        bool IsLoaded() const { return m_loaded; }

    private:
        struct Storage;

        std::unique_ptr<Storage> m_storage;
        bool m_loaded = false;
    };
}
