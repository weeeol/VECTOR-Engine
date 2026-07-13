#pragma once

#include "Engine/Core/Scene.hpp"
#include "Engine/ECS/ECS.hpp"
#include "Game/Core/GameComponents.hpp"
#include "Engine/Graphics/ParticleSystem.hpp"
#include <memory>

namespace VECTOR {
    class InputManager;
}

namespace Game {

    enum class GameState {
        Playing,
        GameOver
    };

    class GameplayScene : public VECTOR::Scene {
    public:
        GameplayScene(int width, int height, VECTOR::InputManager* inputManager, AIDifficulty aiDifficulty);
        ~GameplayScene() override;

        void OnEnter() override;
        void Update(float deltaTime) override;
        void Render(VECTOR::Renderer* renderer) override;

    private:
        void CheckCollisions();
        void ResetGame();

        int m_Width;
        int m_Height;
        VECTOR::InputManager* m_InputManager;

        VECTOR::Registry m_Registry;
        VECTOR::Entity m_Player1;
        VECTOR::Entity m_Player2;
        VECTOR::Entity m_Ball;

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

} // namespace Game
