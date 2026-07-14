#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Engine/Physics/BulletPhysicsSystem.hpp"
#include "Engine/UI/UISystem.hpp"
#include "Game/Systems/GameSystems.hpp"
#include "Game/Systems/CameraSystem.hpp"
#include "Game/Systems/ShootingSystem.hpp"
#include <memory>
#include <vector>

namespace VECTOR {
    class InputManager;
}

namespace Game {

    class GameplayScene : public VECTOR::Scene {
    public:
        GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty);
        ~GameplayScene() override;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void GenerateArena();
        void CreateCube(const glm::vec3& position, const glm::vec3& scale, float mass, const glm::vec3& color, bool isEnemy = false);

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_Registry;
        VECTOR::Entity m_Player;

        std::vector<std::unique_ptr<VECTOR::System>> m_Systems;
        VECTOR::BulletPhysicsSystem* m_PhysicsSystem = nullptr;

        VECTOR::Registry m_UIRegistry;
        std::unique_ptr<VECTOR::UISystem> m_UISystem;

        bool m_IsPaused;
        bool m_WasPausePressed;
        
        bool m_DebugMode = false;
        bool m_WasF3Pressed = false;
        
        std::shared_ptr<VECTOR::Mesh> m_CubeMesh;
    };
}
