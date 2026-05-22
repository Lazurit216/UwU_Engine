#include "uwupch.h"
#include "PhysicsSystem.h"

namespace UwU_Engine
{
    namespace
    {
        Vector3 ScaleOf(const TransformComponent& transform)
        {
            return {
                std::abs(transform.scaleX),
                std::abs(transform.scaleY),
                std::abs(transform.scaleZ)
            };
        }

        void Translate(TransformComponent& transform, const Vector3& delta)
        {
            transform.x += delta.x;
            transform.y += delta.y;
            transform.z += delta.z;
        }

        DirectX::XMMATRIX RotationOf(const TransformComponent& transform)
        {
            return DirectX::XMMatrixRotationRollPitchYaw(
                transform.rotationX,
                transform.rotationY,
                transform.rotationZ);
        }

        Vector3 RotateVector(const Vector3& value, const DirectX::XMMATRIX& rotation)
        {
            const DirectX::XMVECTOR rotated = DirectX::XMVector3TransformNormal(
                DirectX::XMVectorSet(value.x, value.y, value.z, 0.0f),
                rotation);

            DirectX::XMFLOAT3 result;
            DirectX::XMStoreFloat3(&result, rotated);
            return { result.x, result.y, result.z };
        }
    }

    void PhysicsSystem::FixedUpdate(World& world, float fixedDt)
    {
        if (fixedDt <= 0.0f)
            return;

        m_contacts.clear();
        m_grounded.clear();
        m_currentPairs.clear();

        world.ForEach<TransformComponent, RigidbodyComponent>(
            [this, fixedDt](EntityId /*entity*/, TransformComponent& transform, RigidbodyComponent& body)
            {
                if (body.isStatic)
                    return;

                Vector3 acceleration = body.acceleration;
                if (body.useGravity)
                    acceleration += m_gravity;

                body.velocity += acceleration * fixedDt;

                if (body.linearDamping > 0.0f)
                {
                    const float damping = std::clamp(1.0f - body.linearDamping * fixedDt, 0.0f, 1.0f);
                    body.velocity *= damping;
                }

                Translate(transform, body.velocity * fixedDt);
                body.acceleration = {};
            });

        const auto objects = CollectObjects(world);
        for (size_t i = 0; i < objects.size(); ++i)
        {
            for (size_t j = i + 1; j < objects.size(); ++j)
            {
                PhysicsContact contact;
                if (!DetectCollision(objects[i], objects[j], contact))
                    continue;

                contact.trigger = objects[i].collider->isTrigger || objects[j].collider->isTrigger;
                m_contacts.push_back(contact);
                m_currentPairs.insert(MakePairKey(contact.a, contact.b));

                if (!contact.trigger)
                    ResolveCollision(m_contacts.back(), objects[i], objects[j]);
            }
        }

        PublishEvents();
        m_previousPairs = m_currentPairs;
    }

    void PhysicsSystem::AddEventListener(PhysicsEventListener listener)
    {
        m_listeners.push_back(std::move(listener));
    }

    void PhysicsSystem::ClearEventListeners()
    {
        m_listeners.clear();
    }

    bool PhysicsSystem::IsGrounded(EntityId entity) const
    {
        return m_grounded.find(entity) != m_grounded.end();
    }

    Aabb PhysicsSystem::ComputeWorldAabb(const TransformComponent& transform, const ColliderComponent& collider)
    {
        const Vector3 scale = ScaleOf(transform);
        const Vector3 center = ComputeWorldCenter(transform, collider);

        if (collider.type == ColliderType::Sphere)
        {
            const float radius = ComputeWorldSphereRadius(transform, collider);
            return Aabb{ center, Vector3{ radius, radius, radius } };
        }

        const Vector3 localHalf{
            collider.halfExtents.x * scale.x,
            collider.halfExtents.y * scale.y,
            collider.halfExtents.z * scale.z
        };

        const DirectX::XMMATRIX rotation = RotationOf(transform);
        DirectX::XMFLOAT4X4 matrix;
        DirectX::XMStoreFloat4x4(&matrix, rotation);

        return Aabb{
            center,
            Vector3{
                std::abs(matrix._11) * localHalf.x + std::abs(matrix._21) * localHalf.y + std::abs(matrix._31) * localHalf.z,
                std::abs(matrix._12) * localHalf.x + std::abs(matrix._22) * localHalf.y + std::abs(matrix._32) * localHalf.z,
                std::abs(matrix._13) * localHalf.x + std::abs(matrix._23) * localHalf.y + std::abs(matrix._33) * localHalf.z
            }
        };
    }

    Vector3 PhysicsSystem::ComputeWorldCenter(const TransformComponent& transform, const ColliderComponent& collider)
    {
        const Vector3 scale = ScaleOf(transform);
        const Vector3 scaledOffset{
            collider.offset.x * scale.x,
            collider.offset.y * scale.y,
            collider.offset.z * scale.z
        };
        const Vector3 rotatedOffset = RotateVector(scaledOffset, RotationOf(transform));
        return {
            transform.x + rotatedOffset.x,
            transform.y + rotatedOffset.y,
            transform.z + rotatedOffset.z
        };
    }

    float PhysicsSystem::ComputeWorldSphereRadius(const TransformComponent& transform, const ColliderComponent& collider)
    {
        const Vector3 scale = ScaleOf(transform);
        const float maxScale = (std::max)(scale.x, (std::max)(scale.y, scale.z));
        return collider.radius * maxScale;
    }

    std::vector<PhysicsSystem::PhysicsObject> PhysicsSystem::CollectObjects(World& world) const
    {
        std::vector<PhysicsObject> objects;
        world.ForEach<TransformComponent, ColliderComponent>(
            [&objects, &world](EntityId entity, TransformComponent& transform, ColliderComponent& collider)
            {
                objects.push_back(PhysicsObject{
                    entity,
                    &transform,
                    &collider,
                    world.GetComponent<RigidbodyComponent>(entity)
                    });
            });
        return objects;
    }

    bool PhysicsSystem::DetectCollision(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const
    {
        if (!a.collider || !b.collider)
            return false;

        if (a.collider->type == ColliderType::Box && b.collider->type == ColliderType::Box)
            return DetectAabbVsAabb(a, b, outContact);

        if (a.collider->type == ColliderType::Sphere && b.collider->type == ColliderType::Sphere)
            return DetectSphereVsSphere(a, b, outContact);

        if (a.collider->type == ColliderType::Sphere && b.collider->type == ColliderType::Box)
            return DetectSphereVsAabb(a, b, outContact);

        if (a.collider->type == ColliderType::Box && b.collider->type == ColliderType::Sphere)
        {
            if (!DetectSphereVsAabb(b, a, outContact))
                return false;

            std::swap(outContact.a, outContact.b);
            outContact.normal *= -1.0f;
            return true;
        }

        return false;
    }

    bool PhysicsSystem::DetectAabbVsAabb(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const
    {
        const Aabb aabbA = ComputeWorldAabb(*a.transform, *a.collider);
        const Aabb aabbB = ComputeWorldAabb(*b.transform, *b.collider);
        const Vector3 delta = aabbA.center - aabbB.center;

        const float overlapX = (aabbA.halfExtents.x + aabbB.halfExtents.x) - std::abs(delta.x);
        const float overlapY = (aabbA.halfExtents.y + aabbB.halfExtents.y) - std::abs(delta.y);
        const float overlapZ = (aabbA.halfExtents.z + aabbB.halfExtents.z) - std::abs(delta.z);

        if (overlapX <= 0.0f || overlapY <= 0.0f || overlapZ <= 0.0f)
            return false;

        outContact.a = a.entity;
        outContact.b = b.entity;
        outContact.penetration = overlapX;
        outContact.normal = { delta.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f };

        if (overlapY < outContact.penetration)
        {
            outContact.penetration = overlapY;
            outContact.normal = { 0.0f, delta.y >= 0.0f ? 1.0f : -1.0f, 0.0f };
        }

        if (overlapZ < outContact.penetration)
        {
            outContact.penetration = overlapZ;
            outContact.normal = { 0.0f, 0.0f, delta.z >= 0.0f ? 1.0f : -1.0f };
        }

        return true;
    }

    bool PhysicsSystem::DetectSphereVsSphere(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const
    {
        const Vector3 centerA = ComputeWorldCenter(*a.transform, *a.collider);
        const Vector3 centerB = ComputeWorldCenter(*b.transform, *b.collider);
        const float radiusA = ComputeWorldSphereRadius(*a.transform, *a.collider);
        const float radiusB = ComputeWorldSphereRadius(*b.transform, *b.collider);
        const Vector3 delta = centerA - centerB;
        const float distanceSq = LengthSquared(delta);
        const float radiusSum = radiusA + radiusB;

        if (distanceSq >= radiusSum * radiusSum)
            return false;

        const float distance = std::sqrt((std::max)(distanceSq, 0.000001f));
        outContact.a = a.entity;
        outContact.b = b.entity;
        outContact.normal = distance > 0.00001f ? delta / distance : Vector3{ 0.0f, 1.0f, 0.0f };
        outContact.penetration = radiusSum - distance;
        return true;
    }

    bool PhysicsSystem::DetectSphereVsAabb(const PhysicsObject& sphere, const PhysicsObject& box, PhysicsContact& outContact) const
    {
        const Vector3 sphereCenter = ComputeWorldCenter(*sphere.transform, *sphere.collider);
        const float radius = ComputeWorldSphereRadius(*sphere.transform, *sphere.collider);
        const Aabb boxAabb = ComputeWorldAabb(*box.transform, *box.collider);
        const Vector3 closest = Clamp(sphereCenter, boxAabb.Min(), boxAabb.Max());
        Vector3 delta = sphereCenter - closest;
        float distanceSq = LengthSquared(delta);

        if (distanceSq > radius * radius)
            return false;

        outContact.a = sphere.entity;
        outContact.b = box.entity;

        if (distanceSq > 0.000001f)
        {
            const float distance = std::sqrt(distanceSq);
            outContact.normal = delta / distance;
            outContact.penetration = radius - distance;
            return true;
        }

        const Vector3 local = sphereCenter - boxAabb.center;
        const Vector3 remaining{
            boxAabb.halfExtents.x - std::abs(local.x),
            boxAabb.halfExtents.y - std::abs(local.y),
            boxAabb.halfExtents.z - std::abs(local.z)
        };

        outContact.penetration = remaining.x + radius;
        outContact.normal = { local.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f };

        if (remaining.y < remaining.x && remaining.y <= remaining.z)
        {
            outContact.penetration = remaining.y + radius;
            outContact.normal = { 0.0f, local.y >= 0.0f ? 1.0f : -1.0f, 0.0f };
        }
        else if (remaining.z < remaining.x && remaining.z < remaining.y)
        {
            outContact.penetration = remaining.z + radius;
            outContact.normal = { 0.0f, 0.0f, local.z >= 0.0f ? 1.0f : -1.0f };
        }

        return true;
    }

    void PhysicsSystem::ResolveCollision(PhysicsContact& contact, const PhysicsObject& a, const PhysicsObject& b)
    {
        const float invMassA = InverseMass(a);
        const float invMassB = InverseMass(b);
        const float invMassSum = invMassA + invMassB;
        if (invMassSum <= 0.0f)
            return;

        const float slop = 0.001f;
        const float percent = 0.85f;
        const Vector3 correction = contact.normal
            * ((std::max)(contact.penetration - slop, 0.0f) / invMassSum * percent);

        if (invMassA > 0.0f && a.transform)
        {
            const Vector3 delta = correction * invMassA;
            Translate(*a.transform, delta);
            if (delta.y > 0.0001f)
                m_grounded.insert(a.entity);
        }

        if (invMassB > 0.0f && b.transform)
        {
            const Vector3 delta = correction * -invMassB;
            Translate(*b.transform, delta);
            if (delta.y > 0.0001f)
                m_grounded.insert(b.entity);
        }

        Vector3 velocityA = a.rigidbody ? a.rigidbody->velocity : Vector3{};
        Vector3 velocityB = b.rigidbody ? b.rigidbody->velocity : Vector3{};
        const Vector3 relativeVelocity = velocityA - velocityB;
        const float velocityAlongNormal = Dot(relativeVelocity, contact.normal);
        if (velocityAlongNormal > 0.0f)
            return;

        const float restitution = (std::min)(a.collider->bounciness, b.collider->bounciness);
        const float impulseAmount = -(1.0f + restitution) * velocityAlongNormal / invMassSum;
        const Vector3 impulse = contact.normal * impulseAmount;

        if (a.rigidbody && invMassA > 0.0f)
            a.rigidbody->velocity += impulse * invMassA;

        if (b.rigidbody && invMassB > 0.0f)
            b.rigidbody->velocity -= impulse * invMassB;

        velocityA = a.rigidbody ? a.rigidbody->velocity : Vector3{};
        velocityB = b.rigidbody ? b.rigidbody->velocity : Vector3{};
        const Vector3 newRelativeVelocity = velocityA - velocityB;
        Vector3 tangent = newRelativeVelocity - contact.normal * Dot(newRelativeVelocity, contact.normal);
        if (LengthSquared(tangent) <= 0.000001f)
            return;

        tangent = Normalize(tangent);
        const float friction = std::sqrt(a.collider->friction * b.collider->friction);
        const float frictionImpulseAmount = -Dot(newRelativeVelocity, tangent) / invMassSum;
        const Vector3 frictionImpulse = tangent * (frictionImpulseAmount * friction);

        if (a.rigidbody && invMassA > 0.0f)
            a.rigidbody->velocity += frictionImpulse * invMassA;

        if (b.rigidbody && invMassB > 0.0f)
            b.rigidbody->velocity -= frictionImpulse * invMassB;
    }

    void PhysicsSystem::PublishEvents()
    {
        for (const PhysicsContact& contact : m_contacts)
        {
            const PairKey key = MakePairKey(contact.a, contact.b);
            const bool alreadyTouching = m_previousPairs.find(key) != m_previousPairs.end();
            PhysicsEvent event;
            event.contact = contact;
            if (contact.trigger)
                event.kind = alreadyTouching ? PhysicsEventKind::TriggerStay : PhysicsEventKind::TriggerEnter;
            else
                event.kind = alreadyTouching ? PhysicsEventKind::CollisionStay : PhysicsEventKind::CollisionEnter;
            Publish(event);
        }

        for (PairKey key : m_previousPairs)
        {
            if (m_currentPairs.find(key) != m_currentPairs.end())
                continue;

            const EntityId a = static_cast<EntityId>(key >> 32);
            const EntityId b = static_cast<EntityId>(key & 0xffffffffu);
            Publish(PhysicsEvent{ PhysicsEventKind::CollisionExit, PhysicsContact{ a, b } });
        }
    }

    void PhysicsSystem::Publish(const PhysicsEvent& event) const
    {
        for (const auto& listener : m_listeners)
            listener(event);
    }

    bool PhysicsSystem::IsStatic(const PhysicsObject& object)
    {
        return !object.rigidbody || object.rigidbody->isStatic || object.rigidbody->mass <= 0.0f;
    }

    float PhysicsSystem::InverseMass(const PhysicsObject& object)
    {
        return IsStatic(object) ? 0.0f : 1.0f / (std::max)(object.rigidbody->mass, 0.0001f);
    }

    PhysicsSystem::PairKey PhysicsSystem::MakePairKey(EntityId a, EntityId b)
    {
        if (a > b)
            std::swap(a, b);

        return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
    }
}
