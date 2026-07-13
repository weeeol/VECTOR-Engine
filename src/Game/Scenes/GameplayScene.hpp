#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/Physics/BulletPhysicsSystem.hpp"
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
        unsigned int CreateCubeMesh();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_Registry;
        VECTOR::Entity m_Player;

        std::vector<std::unique_ptr<VECTOR::System>> m_Systems;
        VECTOR::BulletPhysicsSystem* m_PhysicsSystem = nullptr;

        VECTOR::UIManager m_PauseMenuUI;

        bool m_IsPaused;
        bool m_WasPausePressed;
        
        unsigned int m_CubeVAO, m_CubeVBO, m_CubeEBO;
        int m_CubeIndexCount;
    };
}
