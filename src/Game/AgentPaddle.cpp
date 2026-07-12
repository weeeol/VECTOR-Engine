#include "AgentPaddle.hpp"
#include <algorithm>

namespace Game {

    AgentPaddle::AgentPaddle(float x, float y)
        // We pass dummy keys since it won't use the standard InputManager update
        : Paddle(x, y, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN),
          m_ReactionDelayTimer(0.0f), m_TargetY(y), m_ReactionTime(0.1f), m_Difficulty(AIDifficulty::Medium)
    {
        SetDifficulty(AIDifficulty::Medium);
    }

    void AgentPaddle::SetDifficulty(AIDifficulty difficulty) {
        m_Difficulty = difficulty;
        switch (m_Difficulty) {
            case AIDifficulty::Easy:
                m_Speed = 250.0f;
                m_ReactionTime = 0.15f;
                break;
            case AIDifficulty::Medium:
                m_Speed = 380.0f;
                m_ReactionTime = 0.05f;
                break;
            case AIDifficulty::Hard:
                m_Speed = 500.0f;
                m_ReactionTime = 0.0f; // Instant reaction
                break;
        }
    }

    void AgentPaddle::UpdateAI(float deltaTime, const Ball* ball, int screenHeight) {
        m_ReactionDelayTimer += deltaTime;

        // Only update our target position based on reaction time
        if (m_ReactionDelayTimer >= m_ReactionTime) {
            VECTOR::AABB ballAABB = ball->GetAABB();
            // Target the center of the ball
            m_TargetY = ballAABB.y + (ballAABB.h / 2.0f);
            
            // Add a little bit of error on easy/medium so it doesn't track perfectly
            if (m_Difficulty == AIDifficulty::Easy) {
                m_TargetY += (rand() % 40) - 20; 
            }

            m_ReactionDelayTimer = 0.0f;
        }

        VECTOR::AABB paddleAABB = GetAABB();
        float paddleCenterY = paddleAABB.y + (paddleAABB.h / 2.0f);

        // Move towards target
        if (paddleCenterY < m_TargetY - 5.0f) {
            m_Position.y += m_Speed * deltaTime;
        } else if (paddleCenterY > m_TargetY + 5.0f) {
            m_Position.y -= m_Speed * deltaTime;
        }

        // Clamp to screen
        if (m_Position.y < 0.0f) {
            m_Position.y = 0.0f;
        }
        if (m_Position.y + paddleAABB.h > static_cast<float>(screenHeight)) {
            m_Position.y = static_cast<float>(screenHeight) - paddleAABB.h;
        }
    }

} // namespace Game
