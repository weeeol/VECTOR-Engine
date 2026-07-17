#include "Engine/Physics/Box2DPhysicsSystem.hpp"
#include "Engine/ECS/Components.hpp"

namespace VECTOR {

    Box2DPhysicsSystem::Box2DPhysicsSystem() {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{0.0f, 0.0f};
        m_WorldId = b2CreateWorld(&worldDef);
    }

    Box2DPhysicsSystem::~Box2DPhysicsSystem() {
        b2DestroyWorld(m_WorldId);
    }

    void Box2DPhysicsSystem::Update(Registry& registry, float deltaTime) {
        b2World_Step(m_WorldId, deltaTime, 4);

        registry.View<TransformComponent, RigidBodyComponent>([&](Entity entity) {
            auto& transform = registry.GetComponent<TransformComponent>(entity);
            auto& rigidBody = registry.GetComponent<RigidBodyComponent>(entity);

            if (b2Body_IsValid(rigidBody.bodyId)) {
                b2Vec2 position = b2Body_GetPosition(rigidBody.bodyId);
                transform.position.x = position.x * PIXELS_PER_METER;
                transform.position.y = position.y * PIXELS_PER_METER;
            }
        });
    }

} // namespace VECTOR
