#pragma once

#include "Engine/ECS/ECS.hpp"
#include "Engine/ECS/System.hpp"
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <memory>

namespace VECTOR {

    class BulletPhysicsSystem : public System {
    public:
        BulletPhysicsSystem();
        ~BulletPhysicsSystem() override;

        void Update(Registry& registry, float deltaTime) override;

        btDiscreteDynamicsWorld* GetWorld() { return m_DynamicsWorld.get(); }

    private:
        std::unique_ptr<btDefaultCollisionConfiguration> m_CollisionConfiguration;
        std::unique_ptr<btCollisionDispatcher> m_Dispatcher;
        std::unique_ptr<btBroadphaseInterface> m_OverlappingPairCache;
        std::unique_ptr<btSequentialImpulseConstraintSolver> m_Solver;
        std::unique_ptr<btDiscreteDynamicsWorld> m_DynamicsWorld;
        std::unique_ptr<btGhostPairCallback> m_GhostPairCallback;
    };

} // namespace VECTOR
