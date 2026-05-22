#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"
#include "Engine/Math/Math.h"

namespace UwU_Engine
{
    enum class ColliderType
    {
        Box,
        Sphere
    };

    struct Aabb
    {
        Vector3 center{ 0.0f, 0.0f, 0.0f };
        Vector3 halfExtents{ 0.5f, 0.5f, 0.5f };

        Vector3 Min() const { return center - halfExtents; }
        Vector3 Max() const { return center + halfExtents; }
    };

    struct PhysicsContact
    {
        EntityId a = kInvalidEntity;
        EntityId b = kInvalidEntity;
        Vector3 normal{ 0.0f, 1.0f, 0.0f };
        float penetration = 0.0f;
        bool trigger = false;
    };

    enum class PhysicsEventKind
    {
        CollisionEnter,
        CollisionStay,
        CollisionExit,
        TriggerEnter,
        TriggerStay,
        TriggerExit
    };

    struct PhysicsEvent
    {
        PhysicsEventKind kind = PhysicsEventKind::CollisionEnter;
        PhysicsContact contact;
    };
}
