#pragma once

#include "Paddle.hpp"
#include "Ball.hpp"

namespace Game {

    enum class AIDifficulty {
        Easy,
        Medium,
        Hard
    };

    class AgentPaddle : public Paddle {
    public:
        AgentPaddle(float x, float y);

        void SetDifficulty(AIDifficulty difficulty);

        // Update based on the ball's position instead of input
        void UpdateAI(float deltaTime, const Ball* ball, int screenHeight);

    private:
        float m_ReactionDelayTimer;
        float m_TargetY;
        
        float m_ReactionTime;
        AIDifficulty m_Difficulty;
    };

} // namespace Game
