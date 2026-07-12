#pragma once

#include "Engine/Core/Application.hpp"
#include "Game/Entities/Paddle.hpp"
#include "Game/Entities/AIPaddle.hpp"
#include "Game/Entities/Ball.hpp"
#include <memory>

namespace Game {

    enum class GameState {
        StartMenu,
        Playing,
        Paused
    };

    class PongGame : public VECTOR::Application {
    public:
        PongGame(const std::string& title, int width, int height);
        ~PongGame();

    protected:
        void Update(float deltaTime) override;
        void Render() override;

    private:
        void CheckCollisions();
        void ResetGame();
        void ToggleDebugMode();

        GameState m_State;
        bool m_WasPausePressed;
        bool m_WasEnterPressed;
        bool m_WasF3Pressed;

        bool m_DebugMode;

        std::unique_ptr<Paddle> m_Player1;
        std::unique_ptr<AIPaddle> m_Player2;
        std::unique_ptr<Ball> m_Ball;

        int m_Score1;
        int m_Score2;
    };

} // namespace Game
