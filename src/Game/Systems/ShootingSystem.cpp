#include "ShootingSystem.hpp"
#include "Engine/ECS/Components.hpp"
#include <iostream>

namespace Game {

    ShootingSystem::ShootingSystem(VECTOR::InputManager* inputManager, VECTOR::BulletPhysicsSystem* physicsSys) 
        : m_InputManager(inputManager), m_PhysicsSystem(physicsSys) {
    }

    ShootingSystem::~ShootingSystem() {
    }

    void ShootingSystem::Update(VECTOR::Registry& registry, float deltaTime) {
        if (m_ShootCooldown > 0.0f) {
            m_ShootCooldown -= deltaTime;
        }

        if (m_InputManager->IsMouseButtonPressed(1) && m_ShootCooldown <= 0.0f) { // Left click
            m_ShootCooldown = 0.5f;

            // Find camera
            VECTOR::Entity cameraEntity = -1;
            registry.View<VECTOR::TransformComponent, VECTOR::CameraComponent>([&](VECTOR::Entity entity) {
                cameraEntity = entity;
            });

            if (cameraEntity == (VECTOR::Entity)-1) return;

            auto& camT = registry.GetComponent<VECTOR::TransformComponent>(cameraEntity);
            auto& camC = registry.GetComponent<VECTOR::CameraComponent>(cameraEntity);

            // Spawn bullet
            VECTOR::Entity bullet = registry.CreateEntity();
            registry.AddComponent(bullet, VECTOR::TransformComponent{camT.position + camC.front * 2.0f});
            
            // Bullet visual size 0.5
            registry.AddComponent(bullet, VECTOR::RenderComponent{0.5f, 0.5f, 255, 255, 0, 255});
            
            // Physical sphere
            btCollisionShape* colShape = new btSphereShape(btScalar(0.5));
            btTransform startTransform;
            startTransform.setIdentity();
            startTransform.setOrigin(btVector3(camT.position.x + camC.front.x * 2.0f, camT.position.y + camC.front.y * 2.0f, camT.position.z + camC.front.z * 2.0f));
            
            btScalar mass(1.0f);
            btVector3 localInertia(0, 0, 0);
            colShape->calculateLocalInertia(mass, localInertia);
            
            btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
            btRigidBody* body = new btRigidBody(rbInfo);
            
            // Shoot forward
            btVector3 velocity = btVector3(camC.front.x, camC.front.y, camC.front.z) * 50.0f;
            body->setLinearVelocity(velocity);
            
            m_PhysicsSystem->GetWorld()->addRigidBody(body);
            registry.AddComponent(bullet, VECTOR::RigidBodyComponent{body});
        }
    }

} // namespace Game
