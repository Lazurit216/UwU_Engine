#pragma once

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS
#endif

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace UwU_Engine
{
    using Vector2 = glm::vec2;
    using Vector3 = glm::vec3;
    using Vector4 = glm::vec4;
    using Matrix4 = glm::mat4;
    using Quaternion = glm::quat;

    inline float Dot(const Vector3& a, const Vector3& b)
    {
        return glm::dot(a, b);
    }

    inline float LengthSquared(const Vector3& value)
    {
        return glm::dot(value, value);
    }

    inline float Length(const Vector3& value)
    {
        return glm::length(value);
    }

    inline Vector3 Normalize(const Vector3& value, const Vector3& fallback = Vector3{ 0.0f, 1.0f, 0.0f })
    {
        return LengthSquared(value) > 0.000001f ? glm::normalize(value) : fallback;
    }

    inline Vector3 Clamp(const Vector3& value, const Vector3& minValue, const Vector3& maxValue)
    {
        return glm::clamp(value, minValue, maxValue);
    }

    inline Matrix4 RotationMatrix(float rotationX, float rotationY, float rotationZ)
    {
        Matrix4 rotation{ 1.0f };
        rotation = glm::rotate(rotation, rotationZ, Vector3{ 0.0f, 0.0f, 1.0f });
        rotation = glm::rotate(rotation, rotationY, Vector3{ 0.0f, 1.0f, 0.0f });
        rotation = glm::rotate(rotation, rotationX, Vector3{ 1.0f, 0.0f, 0.0f });
        return rotation;
    }

    inline Vector3 TransformDirection(const Matrix4& matrix, const Vector3& direction)
    {
        return Vector3{ matrix * Vector4{ direction, 0.0f } };
    }
}
