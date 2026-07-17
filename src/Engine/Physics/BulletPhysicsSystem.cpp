#include "Engine/Physics/BulletPhysicsSystem.hpp"
#include "Engine/ECS/Components.hpp"

namespace VECTOR {

    BulletPhysicsSystem::BulletPhysicsSystem() {
        m_CollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
        m_Dispatcher = std::make_unique<btCollisionDispatcher>(m_CollisionConfiguration.get());
        m_OverlappingPairCache = std::make_unique<btDbvtBroadphase>();
        m_Solver = std::make_unique<btSequentialImpulseConstraintSolver>();

        m_DynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
            m_Dispatcher.get(),
            m_OverlappingPairCache.get(),
            m_Solver.get(),
            m_CollisionConfiguration.get()
        );

        m_DynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    }

    BulletPhysicsSystem::~BulletPhysicsSystem() {
        if (m_DynamicsWorld) {
            int remaining = m_DynamicsWorld->getNumCollisionObjects();
            if (remaining > 0) {
                // Should not happen if ECS components cleaned up properly
                // But if it does, log or handle
            }
        }
    }

    void BulletPhysicsSystem::Update(Registry& registry, float deltaTime) {
        m_DynamicsWorld->stepSimulation(deltaTime, 10);

        registry.View<TransformComponent, RigidBodyComponent>([&](Entity entity) {
            auto& rb = registry.GetComponent<RigidBodyComponent>(entity);
            auto& t = registry.GetComponent<TransformComponent>(entity);

            if (rb.body && rb.body->getMotionState()) {
                btTransform trans;
                rb.body->getMotionState()->getWorldTransform(trans);
                t.position.x = trans.getOrigin().getX();
                t.position.y = trans.getOrigin().getY();
                t.position.z = trans.getOrigin().getZ();

                t.rotation.w = trans.getRotation().getW();
                t.rotation.x = trans.getRotation().getX();
                t.rotation.y = trans.getRotation().getY();
                t.rotation.z = trans.getRotation().getZ();
            }
        });
    }

} // namespace VECTOR
