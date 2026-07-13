#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Game/Systems/GameSystems.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/Physics/Box2DPhysicsSystem.hpp"
#include <box2d/box2d.h>
#include <memory>
#include <vector>

namespace VECTOR {
    class InputManager;
}

namespace Game {
    enum class GameState { Playing, GameOver };

    class GameplayScene : public VECTOR::Scene {
    public:
        GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty);
        ~GameplayScene() override;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void ResetGame();
        b2BodyId CreateBox(float x, float y, float width, float height, b2BodyType type, float density, float friction, float restitution, bool isSensor, void* userData);

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_Registry;
        VECTOR::Entity m_Player1;
        VECTOR::Entity m_Player2;
        VECTOR::Entity m_Ball;

        std::vector<std::unique_ptr<VECTOR::System>> m_Systems;
        BallMechanicsSystem* m_BallSystem = nullptr;
        VECTOR::Box2DPhysicsSystem* m_PhysicsSystem = nullptr;
        
        VECTOR::UIManager m_PauseMenuUI;

        int m_Score1;
        int m_Score2;
        bool m_IsPaused;
        bool m_WasPausePressed;
        bool m_DebugMode;
        bool m_WasF3Pressed;

        VECTOR::ParticleEmitter m_TrailEmitter;
        VECTOR::ParticleEmitter m_ExplosionEmitter;
        
        GameState m_State;
        int m_Winner; // 1 for Player 1, 2 for Player 2
        const int WINNING_SCORE = 5;
    };
}
