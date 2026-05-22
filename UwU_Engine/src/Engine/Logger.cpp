#include "Logger.h"
#include "uwupch.h"

namespace UwU_Engine
{
    namespace fs = std::filesystem;

    Logger* Logger::s_EngineLogger = nullptr;
    Logger* Logger::s_ClientLogger = nullptr;
    std::vector<Logger::Entry> Logger::s_entries;
    std::mutex Logger::s_entriesMutex;

    namespace
    {
        constexpr size_t kMaxBufferedLogEntries = 2000;
    }

    // Helpers
    bool Logger::EnsureLogsCacheDirectory()
    {
        try
        {
            fs::path logsCachePath = fs::current_path() / "LogsCache";
            if (!fs::exists(logsCachePath))
                fs::create_directories(logsCachePath);
            return true;
        }
        catch (const std::exception& e)
        {
            std::cerr << std::format("[Logger] Failed to create LogsCache directory: {}\n", e.what());
            return false;
        }
    }

    std::string Logger::GetLogFilePath(const std::string& fileName)
    {
        try
        {
            fs::path logsCachePath = fs::current_path() / "LogsCache" / fileName;
            return logsCachePath.string();
        }
        catch (const std::exception& e)
        {
            std::cerr << std::format("[Logger] Failed to construct log file path: {}\n", e.what());
            return "";
        }
    }

    // Constructor / Destructor
    Logger::Logger(const std::string& name, const std::string& filePath, Level minLevel)
        : m_name(name), m_minLevel(minLevel)
    {
        if (!filePath.empty())
        {
            if (!EnsureLogsCacheDirectory())
            {
                OutputDebugStringA(std::format("[{}] Warning: Could not ensure LogsCache directory\n", m_name).c_str());
                return;
            }

            std::string fullPath = GetLogFilePath(filePath);
            if (fullPath.empty())
            {
                OutputDebugStringA(std::format("[{}] Error: Could not construct log file path\n", m_name).c_str());
                return;
            }

            m_file.open(fullPath, std::ios::out | std::ios::app);
            if (!m_file.is_open())
            {
                std::string errorMsg = std::format("[{}] Could not open log file: {}\n", m_name, fullPath);
                std::cerr << errorMsg;
                OutputDebugStringA(errorMsg.c_str());
            }
            else
            {
                std::ostringstream initMsg;
                initMsg << std::format("=== {} Logger Initialized: {} ===\n", m_name, fullPath);
                m_file << initMsg.str();
                m_file.flush();
            }
        }
    }

    Logger::~Logger()
    {
        if (m_file.is_open())
        {
            m_file << std::format("=== {} Logger Shutdown ===\n", m_name);
            m_file.flush();
            m_file.close();
        }
    }

    // Write
    void Logger::Write(Level level, const std::string& message)
    {
        if (static_cast<int>(level) < static_cast<int>(m_minLevel)) return;

        // Timestamp
        auto now = std::chrono::system_clock::now();
        auto ttime = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &ttime);
#else
        localtime_r(&ttime, &tm);
#endif

        std::ostringstream line;
        line << std::put_time(&tm, "%H:%M:%S")
            << '.'
            << std::setw(3) << std::setfill('0') << ms.count()
            << " [" << LevelTag(level) << ']'
            << " [" << m_name << "] "
            << message
            << '\n';

        std::string formatted = line.str();

        std::lock_guard lock(m_mutex);

        std::cout << formatted;
        OutputDebugStringA(formatted.c_str());

        {
            std::lock_guard entriesLock(s_entriesMutex);
            s_entries.push_back(Entry{ level, m_name, message, formatted });
            if (s_entries.size() > kMaxBufferedLogEntries)
                s_entries.erase(s_entries.begin(), s_entries.begin() + (s_entries.size() - kMaxBufferedLogEntries));
        }

        if (m_file.is_open())
        {
            m_file << formatted;
            m_file.flush();
        }
    }

    std::vector<Logger::Entry> Logger::GetEntries()
    {
        std::lock_guard lock(s_entriesMutex);
        return s_entries;
    }

    // Static Init / Shutdown
    void Logger::Init()
    {
        s_EngineLogger = new Logger("ENGINE", "Engine.log", Level::Trace);
        s_ClientLogger = new Logger("CLIENT", "Client.log", Level::Trace);
    }

    void Logger::Shutdown()
    {
        delete s_EngineLogger;
        s_EngineLogger = nullptr;

        delete s_ClientLogger;
        s_ClientLogger = nullptr;
    }
}
