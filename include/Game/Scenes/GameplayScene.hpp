#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Game/Systems/GameSystems.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include "Engine/UI/UIManager.hpp"
#include "Engine/Physics/Box2DPhysicsSystem.hpp"
#include "Engine/Graphics/Texture.hpp"
#include "Engine/Graphics/Animator.hpp"
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
        GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty, GameMode mode);
        ~GameplayScene() override;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void ResetGame();
        void SetupUI();

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
        GameMode m_Mode;
        int m_Winner; // 1 for Player 1, 2 for Player 2
        const int WINNING_SCORE = 5;

        std::shared_ptr<VECTOR::Texture> m_BallTexture;
        std::shared_ptr<VECTOR::Animator> m_BallAnimator;

        // Save Data
        int m_HighScorePlayer;
        int m_HighScoreAI;

        // Juice
        float m_ShakeTimer = 0.0f;
        float m_ShakeMagnitude = 0.0f;
        float m_BackgroundOffset = 0.0f;
        float m_GameOverTimer = 0.0f;

        // Post-Processing
        std::unique_ptr<VECTOR::Texture> m_SceneTexture;
    };
}
