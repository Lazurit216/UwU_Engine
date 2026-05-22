#pragma once

#include "Engine/ECS/Components.h"
#include "Engine/ECS/World.h"

namespace UwU_Engine
{
    inline Matrix4 BuildLocalTransformMatrix(const TransformComponent& transform)
    {
        Matrix4 matrix{ 1.0f };
        matrix = glm::translate(matrix, Vector3{ transform.x, transform.y, transform.z });
        matrix *= RotationMatrix(transform.rotationX, transform.rotationY, transform.rotationZ);
        matrix = glm::scale(matrix, Vector3{ transform.scaleX, transform.scaleY, transform.scaleZ });
        return matrix;
    }

    inline TransformComponent DecomposeTransformMatrix(const Matrix4& matrix)
    {
        TransformComponent transform;
        transform.x = matrix[3].x;
        transform.y = matrix[3].y;
        transform.z = matrix[3].z;

        Vector3 axisX{ matrix[0].x, matrix[0].y, matrix[0].z };
        Vector3 axisY{ matrix[1].x, matrix[1].y, matrix[1].z };
        Vector3 axisZ{ matrix[2].x, matrix[2].y, matrix[2].z };
        transform.scaleX = Length(axisX);
        transform.scaleY = Length(axisY);
        transform.scaleZ = Length(axisZ);

        if (transform.scaleX > 0.000001f) axisX /= transform.scaleX;
        if (transform.scaleY > 0.000001f) axisY /= transform.scaleY;
        if (transform.scaleZ > 0.000001f) axisZ /= transform.scaleZ;

        Matrix4 rotation{ 1.0f };
        rotation[0] = Vector4{ axisX, 0.0f };
        rotation[1] = Vector4{ axisY, 0.0f };
        rotation[2] = Vector4{ axisZ, 0.0f };

        const Vector3 euler = glm::eulerAngles(glm::quat_cast(rotation));
        transform.rotationX = euler.x;
        transform.rotationY = euler.y;
        transform.rotationZ = euler.z;
        return transform;
    }

    inline Matrix4 BuildWorldTransformMatrix(const World& world, EntityId entity, const TransformComponent& local, int depth = 0)
    {
        if (depth > 32)
            return BuildLocalTransformMatrix(local);

        const HierarchyComponent* hierarchy = world.GetComponent<HierarchyComponent>(entity);
        if (!hierarchy || hierarchy->parent == kInvalidEntity || !world.IsAlive(hierarchy->parent))
            return BuildLocalTransformMatrix(local);

        const TransformComponent* parentTransform = world.GetComponent<TransformComponent>(hierarchy->parent);
        if (!parentTransform)
            return BuildLocalTransformMatrix(local);

        return BuildWorldTransformMatrix(world, hierarchy->parent, *parentTransform, depth + 1)
            * BuildLocalTransformMatrix(local);
    }

    inline TransformComponent BuildWorldTransform(const World& world, EntityId entity, const TransformComponent& local)
    {
        return DecomposeTransformMatrix(BuildWorldTransformMatrix(world, entity, local));
    }
}
