#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"

#include <memory>
#include <typeindex>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace UwU_Engine
{
    class IComponentStorage
    {
    public:
        virtual ~IComponentStorage() = default;

        virtual void Remove(EntityId entity) = 0;
        virtual bool Has(EntityId entity) const = 0;
        virtual void Clear() = 0;
    };

    template<typename T>
    class ComponentStorage final : public IComponentStorage
    {
    public:
        template<typename... Args>
        T& Add(EntityId entity, Args&&... args)
        {
            m_components.erase(entity);
            auto [it, inserted] = m_components.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(entity),
                std::forward_as_tuple(std::forward<Args>(args)...));
            return it->second;
        }

        T* Get(EntityId entity)
        {
            auto it = m_components.find(entity);
            return it != m_components.end() ? &it->second : nullptr;
        }

        const T* Get(EntityId entity) const
        {
            auto it = m_components.find(entity);
            return it != m_components.end() ? &it->second : nullptr;
        }

        void Remove(EntityId entity) override
        {
            m_components.erase(entity);
        }

        bool Has(EntityId entity) const override
        {
            return m_components.find(entity) != m_components.end();
        }

        void Clear() override
        {
            m_components.clear();
        }

        std::unordered_map<EntityId, T>& All() { return m_components; }
        const std::unordered_map<EntityId, T>& All() const { return m_components; }

    private:
        std::unordered_map<EntityId, T> m_components;
    };

    class UWU_API World
    {
    public:
        World() = default;
        ~World() = default;

        World(const World&) = delete;
        World& operator=(const World&) = delete;
        World(World&&) noexcept = default;
        World& operator=(World&&) noexcept = default;

        EntityId CreateEntity()
        {
            EntityId entity = m_nextEntity++;
            m_alive.insert(entity);
            return entity;
        }

        void DestroyEntity(EntityId entity)
        {
            if (entity == kInvalidEntity) return;

            m_alive.erase(entity);
            for (auto& [_, storage] : m_storages)
                storage->Remove(entity);
        }

        bool IsAlive(EntityId entity) const
        {
            return m_alive.find(entity) != m_alive.end();
        }

        size_t EntityCount() const
        {
            return m_alive.size();
        }

        void Clear()
        {
            for (auto& [_, storage] : m_storages)
                storage->Clear();
            m_alive.clear();
            m_nextEntity = 1;
        }

        template<typename T, typename... Args>
        T& AddComponent(EntityId entity, Args&&... args)
        {
            return GetOrCreateStorage<T>().Add(entity, std::forward<Args>(args)...);
        }

        template<typename T>
        void RemoveComponent(EntityId entity)
        {
            if (auto* storage = GetStorage<T>())
                storage->Remove(entity);
        }

        template<typename T>
        bool HasComponent(EntityId entity) const
        {
            if (auto* storage = GetStorage<T>())
                return storage->Has(entity);
            return false;
        }

        template<typename T>
        T* GetComponent(EntityId entity)
        {
            auto* storage = GetStorage<T>();
            return storage ? storage->Get(entity) : nullptr;
        }

        template<typename T>
        const T* GetComponent(EntityId entity) const
        {
            auto* storage = GetStorage<T>();
            return storage ? storage->Get(entity) : nullptr;
        }

        template<typename First, typename... Rest, typename Func>
        void ForEach(Func&& func)
        {
            auto* firstStorage = GetStorage<First>();
            if (!firstStorage) return;

            for (auto& [entity, first] : firstStorage->All())
            {
                if (!IsAlive(entity)) continue;
                if constexpr (sizeof...(Rest) == 0)
                {
                    func(entity, first);
                }
                else
                {
                    if ((HasComponent<Rest>(entity) && ...))
                        func(entity, first, *GetComponent<Rest>(entity)...);
                }
            }
        }

    private:
        template<typename T>
        ComponentStorage<T>& GetOrCreateStorage()
        {
            std::type_index key(typeid(T));
            auto it = m_storages.find(key);
            if (it == m_storages.end())
            {
                auto storage = std::make_unique<ComponentStorage<T>>();
                auto* raw = storage.get();
                m_storages.emplace(key, std::move(storage));
                return *raw;
            }
            return *static_cast<ComponentStorage<T>*>(it->second.get());
        }

        template<typename T>
        ComponentStorage<T>* GetStorage()
        {
            std::type_index key(typeid(T));
            auto it = m_storages.find(key);
            return it != m_storages.end()
                ? static_cast<ComponentStorage<T>*>(it->second.get())
                : nullptr;
        }

        template<typename T>
        const ComponentStorage<T>* GetStorage() const
        {
            std::type_index key(typeid(T));
            auto it = m_storages.find(key);
            return it != m_storages.end()
                ? static_cast<const ComponentStorage<T>*>(it->second.get())
                : nullptr;
        }

        EntityId m_nextEntity = 1;
        std::unordered_set<EntityId> m_alive;
        std::unordered_map<std::type_index, std::unique_ptr<IComponentStorage>> m_storages;
    };
}
