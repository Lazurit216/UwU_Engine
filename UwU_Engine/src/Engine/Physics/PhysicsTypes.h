#pragma once

#include "Engine/Core.h"
#include "Engine/ECS/Entity.h"

namespace UwU_Engine
{
    struct Vector3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vector3() = default;
        Vector3(float xValue, float yValue, float zValue)
            : x(xValue), y(yValue), z(zValue)
        {
        }

        Vector3 operator+(const Vector3& other) const { return { x + other.x, y + other.y, z + other.z }; }
        Vector3 operator-(const Vector3& other) const { return { x - other.x, y - other.y, z - other.z }; }
        Vector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
        Vector3 operator/(float scalar) const { return scalar != 0.0f ? Vector3{ x / scalar, y / scalar, z / scalar } : Vector3{}; }

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x;
            y += other.y;
            z += other.z;
            return *this;
        }

        Vector3& operator-=(const Vector3& other)
        {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return *this;
        }

        Vector3& operator*=(float scalar)
        {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return *this;
        }
    };

    inline Vector3 operator*(float scalar, const Vector3& value)
    {
        return value * scalar;
    }

    inline float Dot(const Vector3& a, const Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline float LengthSquared(const Vector3& value)
    {
        return Dot(value, value);
    }

    inline float Length(const Vector3& value)
    {
        return std::sqrt(LengthSquared(value));
    }

    inline Vector3 Normalize(const Vector3& value)
    {
        const float len = Length(value);
        return len > 0.00001f ? value / len : Vector3{ 0.0f, 1.0f, 0.0f };
    }

    inline Vector3 Clamp(const Vector3& value, const Vector3& minValue, const Vector3& maxValue)
    {
        return {
            std::clamp(value.x, minValue.x, maxValue.x),
            std::clamp(value.y, minValue.y, maxValue.y),
            std::clamp(value.z, minValue.z, maxValue.z)
        };
    }

    enum class ColliderType
    {
        Box,
        Sphere
    };

    struct Aabb
    {
        Vector3 center;
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
