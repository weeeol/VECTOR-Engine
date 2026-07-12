#pragma once

#include "Game/Entities/Paddle.hpp"
#include "Game/Entities/Ball.hpp"

namespace Game {

    enum class AIDifficulty {
        Easy,
        Medium,
        Hard
    };

    class AIPaddle : public Paddle {
    public:
        AIPaddle(float x, float y);

        void SetDifficulty(AIDifficulty difficulty);
        AIDifficulty GetDifficulty() const { return m_Difficulty; }

        // Update based on the ball's position instead of input
        void UpdateAI(float deltaTime, const Ball* ball, int screenHeight);

    private:
        float m_ReactionDelayTimer;
        float m_TargetY;
        
        float m_ReactionTime;
        AIDifficulty m_Difficulty;
    };

} // namespace Game
