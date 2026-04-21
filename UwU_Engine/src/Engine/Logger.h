#pragma once
#include "Core.h"

namespace UwU_Engine
{
    class UWU_API Logger
    {
    public:
        enum class Level { Trace, Info, Warn, Error };

        // Instance constructor
        Logger(const std::string& name, const std::string& filePath = "", Level minLevel = Level::Trace);
        ~Logger();

        // Per-logger logging (use via the static pointers below)
        template<typename... Args>
        void Trace(std::format_string<Args...> fmt, Args&&... args)
        {
            Write(Level::Trace, std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Info(std::format_string<Args...> fmt, Args&&... args)
        {
            Write(Level::Info, std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Warn(std::format_string<Args...> fmt, Args&&... args)
        {
            Write(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
        }

        template<typename... Args>
        void Error(std::format_string<Args...> fmt, Args&&... args)
        {
            Write(Level::Error, std::format(fmt, std::forward<Args>(args)...));
        }

        void Trace(const std::string& msg) { Write(Level::Trace, msg); }
        void Info(const std::string& msg) { Write(Level::Info, msg); }
        void Warn(const std::string& msg) { Write(Level::Warn, msg); }
        void Error(const std::string& msg) { Write(Level::Error, msg); }

        static void Init();
        static void Shutdown();

        static Logger* GetEngineLogger() { return s_EngineLogger; }
        static Logger* GetClientLogger() { return s_ClientLogger; }

    private:
        void Write(Level level, const std::string& message);

        static constexpr const char* LevelTag(Level l)
        {
            switch (l)
            {
            case Level::Trace: return "TRACE";
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            }
            return "?????";
        }

        // Instance data
        std::string     m_name;
        std::ofstream   m_file;
        Level           m_minLevel;
        std::mutex      m_mutex;

        // Loggers
        static Logger* s_EngineLogger;
        static Logger* s_ClientLogger;

        // Helpers (kept static)
        static bool EnsureLogsCacheDirectory();
        static std::string GetLogFilePath(const std::string& fileName);
    };
}

// Engine log macros
#define UWU_ENGINE_TRACE(...) ::UwU_Engine::Logger::GetEngineLogger()->Trace(__VA_ARGS__)
#define UWU_ENGINE_INFO(...)  ::UwU_Engine::Logger::GetEngineLogger()->Info(__VA_ARGS__)
#define UWU_ENGINE_WARN(...)  ::UwU_Engine::Logger::GetEngineLogger()->Warn(__VA_ARGS__)
#define UWU_ENGINE_ERROR(...) ::UwU_Engine::Logger::GetEngineLogger()->Error(__VA_ARGS__)

// Client log macros
#define UWU_TRACE(...) ::UwU_Engine::Logger::GetClientLogger()->Trace(__VA_ARGS__)
#define UWU_INFO(...)  ::UwU_Engine::Logger::GetClientLogger()->Info(__VA_ARGS__)
#define UWU_WARN(...)  ::UwU_Engine::Logger::GetClientLogger()->Warn(__VA_ARGS__)
#define UWU_ERROR(...) ::UwU_Engine::Logger::GetClientLogger()->Error(__VA_ARGS__)