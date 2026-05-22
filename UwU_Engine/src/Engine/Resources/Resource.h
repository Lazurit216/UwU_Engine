#pragma once

#include "Engine/Core.h"

namespace UwU_Engine
{
    enum class ResourceState
    {
        Unloaded,
        Loading,
        Loaded,
        Failed
    };

    class IResource
    {
    public:
        virtual ~IResource() = default;

        const std::string& GetPath() const { return m_path; }

        ResourceState GetState() const
        {
            std::scoped_lock lock(m_stateMutex);
            return m_state;
        }

        std::string GetError() const
        {
            std::scoped_lock lock(m_stateMutex);
            return m_error;
        }

        bool IsLoaded() const { return GetState() == ResourceState::Loaded; }
        bool IsLoading() const { return GetState() == ResourceState::Loading; }
        bool IsFailed() const { return GetState() == ResourceState::Failed; }

    protected:
        void SetState(ResourceState state, std::string error = {})
        {
            std::scoped_lock lock(m_stateMutex);
            m_state = state;
            m_error = std::move(error);
        }

        std::string m_path;
        mutable std::mutex m_stateMutex;
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

        void MarkLoading()
        {
            SetState(ResourceState::Loading);
        }

        void MarkLoaded(std::filesystem::file_time_type lastWriteTime = {})
        {
            m_lastWriteTime = lastWriteTime;
            SetState(ResourceState::Loaded);
        }

        void SetDataLoaded(T data, std::filesystem::file_time_type lastWriteTime = {})
        {
            m_data = std::move(data);
            MarkLoaded(lastWriteTime);
        }

        void MarkFailed(std::string error)
        {
            SetState(ResourceState::Failed, std::move(error));
        }

    private:
        T m_data;
        std::filesystem::file_time_type m_lastWriteTime{};
    };
}
