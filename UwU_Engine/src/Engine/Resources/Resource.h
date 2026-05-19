#pragma once

#include "Engine/Core.h"

namespace UwU_Engine
{
    enum class ResourceState
    {
        Unloaded,
        Loaded,
        Failed
    };

    class IResource
    {
    public:
        virtual ~IResource() = default;

        const std::string& GetPath() const { return m_path; }
        ResourceState GetState() const { return m_state; }
        const std::string& GetError() const { return m_error; }
        bool IsLoaded() const { return m_state == ResourceState::Loaded; }

    protected:
        std::string m_path;
        ResourceState m_state = ResourceState::Unloaded;
        std::string m_error;
    };

    template<typename T>
    class Resource final : public IResource
    {
    public:
        explicit Resource(std::string path)
        {
            m_path = std::move(path);
        }

        T& GetData() { return m_data; }
        const T& GetData() const { return m_data; }

        std::filesystem::file_time_type GetLastWriteTime() const { return m_lastWriteTime; }

        void MarkLoaded(std::filesystem::file_time_type lastWriteTime = {})
        {
            m_state = ResourceState::Loaded;
            m_error.clear();
            m_lastWriteTime = lastWriteTime;
        }

        void MarkFailed(std::string error)
        {
            m_state = ResourceState::Failed;
            m_error = std::move(error);
        }

    private:
        T m_data;
        std::filesystem::file_time_type m_lastWriteTime{};
    };
}
