#pragma once

#include "Engine/ECS/Components.h"
#include "Engine/ECS/System.h"
#include "Engine/Physics/PhysicsTypes.h"

namespace UwU_Engine
{
    class PhysicsSystem final : public ISystem
    {
    public:
        using PhysicsEventListener = std::function<void(const PhysicsEvent&)>;

        void Update(World& world, float dt) override { FixedUpdate(world, dt); }
        void FixedUpdate(World& world, float fixedDt);

        void SetGravity(Vector3 gravity) { m_gravity = gravity; }
        void AddEventListener(PhysicsEventListener listener);
        void ClearEventListeners();

        const std::vector<PhysicsContact>& GetContacts() const { return m_contacts; }
        bool IsGrounded(EntityId entity) const;

        static Aabb ComputeWorldAabb(const TransformComponent& transform, const ColliderComponent& collider);
        static Vector3 ComputeWorldCenter(const TransformComponent& transform, const ColliderComponent& collider);
        static float ComputeWorldSphereRadius(const TransformComponent& transform, const ColliderComponent& collider);

    private:
        struct PhysicsObject
        {
            EntityId entity = kInvalidEntity;
            TransformComponent* transform = nullptr;
            TransformComponent worldTransform;
            ColliderComponent* collider = nullptr;
            RigidbodyComponent* rigidbody = nullptr;
        };

        using PairKey = uint64_t;

        std::vector<PhysicsObject> CollectObjects(World& world) const;
        bool DetectCollision(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const;
        bool DetectAabbVsAabb(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const;
        bool DetectSphereVsSphere(const PhysicsObject& a, const PhysicsObject& b, PhysicsContact& outContact) const;
        bool DetectSphereVsBox(const PhysicsObject& sphere, const PhysicsObject& box, PhysicsContact& outContact) const;

        void ResolveCollision(PhysicsContact& contact, const PhysicsObject& a, const PhysicsObject& b);
        void PublishEvents();
        void Publish(const PhysicsEvent& event) const;

        static bool IsStatic(const PhysicsObject& object);
        static float InverseMass(const PhysicsObject& object);
        static PairKey MakePairKey(EntityId a, EntityId b);

    private:
        Vector3 m_gravity{ 0.0f, -9.81f, 0.0f };
        std::vector<PhysicsContact> m_contacts;
        std::unordered_set<EntityId> m_grounded;
        std::unordered_set<PairKey> m_previousPairs;
        std::unordered_set<PairKey> m_currentPairs;
        std::vector<PhysicsEventListener> m_listeners;
    };
}
