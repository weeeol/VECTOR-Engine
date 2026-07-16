#pragma once

#include "Engine/ECS/ECS.hpp"
#include "Engine/ECS/System.hpp"
#include "Engine/Input/InputManager.hpp"
#include "Engine/Physics/BulletPhysicsSystem.hpp"

namespace VECTOR { class Mesh; class Material; }

namespace Game {

    class ShootingSystem : public VECTOR::System {
    public:
        ShootingSystem(VECTOR::InputManager* inputManager, VECTOR::BulletPhysicsSystem* physicsSys);
        ~ShootingSystem() override;

        void Update(VECTOR::Registry& registry, float deltaTime) override;

    private:
        VECTOR::InputManager* m_InputManager;
        VECTOR::BulletPhysicsSystem* m_PhysicsSystem;
        float m_ShootCooldown = 0.0f;
        std::shared_ptr<VECTOR::Mesh> m_BulletMesh;
        std::shared_ptr<VECTOR::Material> m_BulletMaterial;
    };

} // namespace Game

