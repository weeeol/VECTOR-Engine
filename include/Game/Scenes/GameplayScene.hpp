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
    class Material;
    class VulkanCubemap;
}

namespace Game {

    class GameplayScene : public VECTOR::Scene {
    public:
        GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty);
        ~GameplayScene() override;

        void OnEnter() override;
        void OnResize(int width, int height) override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void GenerateArena();
        void CreateCube(const glm::vec3& position, const glm::vec3& scale, float mass, const std::string& materialPath, bool isEnemy = false);
        void CreateUI();
        void ClearUI();

        enum class GameState {
            Playing,
            Paused,
            Settings
        };

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_Registry;
        VECTOR::Entity m_Player;

        std::vector<std::unique_ptr<VECTOR::System>> m_Systems;
        VECTOR::BulletPhysicsSystem* m_PhysicsSystem = nullptr;
        CameraSystem* m_CameraSystem = nullptr;

        VECTOR::Registry m_UIRegistry;
        std::unique_ptr<VECTOR::UISystem> m_UISystem;

        GameState m_State = GameState::Playing;
        bool m_WasEscapePressed = false;
        bool m_NeedsUIRefresh = false;
        
        std::shared_ptr<VECTOR::VulkanCubemap> m_Skybox;
        
        bool m_DebugMode = false;
        bool m_WasF3Pressed = false;
        
        std::shared_ptr<VECTOR::Mesh> m_CubeMesh;

        // Unlit material for objects that shouldn't receive lighting (sun)
        std::shared_ptr<VECTOR::Material> m_UnlitMaterial;
    };
}
