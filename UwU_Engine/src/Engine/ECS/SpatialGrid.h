#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"

namespace UwU_Engine
{
    struct SpatialBounds2D
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;

        bool Intersects(const SpatialBounds2D& other) const
        {
            return maxX >= other.minX && minX <= other.maxX
                && maxY >= other.minY && minY <= other.maxY;
        }
    };

    class SpatialGrid
    {
    public:
        explicit SpatialGrid(float cellSize = 1.0f)
            : m_cellSize((std::max)(cellSize, 0.01f))
        {
        }

        void Clear()
        {
            m_cells.clear();
            m_bounds.clear();
        }

        void Insert(EntityId entity, const SpatialBounds2D& bounds)
        {
            m_bounds[entity] = bounds;

            const int minCellX = ToCell(bounds.minX);
            const int maxCellX = ToCell(bounds.maxX);
            const int minCellY = ToCell(bounds.minY);
            const int maxCellY = ToCell(bounds.maxY);

            for (int y = minCellY; y <= maxCellY; ++y)
                for (int x = minCellX; x <= maxCellX; ++x)
                    m_cells[MakeKey(x, y)].push_back(entity);
        }

        std::vector<EntityId> Query(const SpatialBounds2D& area) const
        {
            std::vector<EntityId> result;
            std::unordered_set<EntityId> unique;

            const int minCellX = ToCell(area.minX);
            const int maxCellX = ToCell(area.maxX);
            const int minCellY = ToCell(area.minY);
            const int maxCellY = ToCell(area.maxY);

            for (int y = minCellY; y <= maxCellY; ++y)
            {
                for (int x = minCellX; x <= maxCellX; ++x)
                {
                    auto it = m_cells.find(MakeKey(x, y));
                    if (it == m_cells.end())
                        continue;

                    for (EntityId entity : it->second)
                    {
                        if (unique.find(entity) != unique.end())
                            continue;

                        auto boundsIt = m_bounds.find(entity);
                        if (boundsIt != m_bounds.end() && boundsIt->second.Intersects(area))
                        {
                            unique.insert(entity);
                            result.push_back(entity);
                        }
                    }
                }
            }

            return result;
        }

    private:
        int ToCell(float value) const
        {
            return static_cast<int>(std::floor(value / m_cellSize));
        }

        static uint64_t MakeKey(int x, int y)
        {
            return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32)
                | static_cast<uint32_t>(y);
        }

        float m_cellSize = 1.0f;
        std::unordered_map<uint64_t, std::vector<EntityId>> m_cells;
        std::unordered_map<EntityId, SpatialBounds2D> m_bounds;
    };
}
