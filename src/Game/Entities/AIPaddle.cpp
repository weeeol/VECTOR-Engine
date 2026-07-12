#include "AIPaddle.hpp"
#include <algorithm>

namespace Game {

    AIPaddle::AIPaddle(float x, float y)
        // We pass dummy keys since it won't use the standard InputManager update
        : Paddle(x, y, SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN),
          m_ReactionDelayTimer(0.0f), m_TargetY(y), m_ReactionTime(0.2f), m_Difficulty(AIDifficulty::Medium), m_State(AIState::Idle)
    {
        m_ID = 2;
        m_Name = "AI_Player2";
        SetDifficulty(AIDifficulty::Medium);
    }

    void AIPaddle::SetDifficulty(AIDifficulty difficulty) {
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

    void AIPaddle::UpdateAI(float deltaTime, const Ball* ball, int screenHeight) {
        // State Machine Logic
        VECTOR::AABB ballAABB = ball->GetAABB();
        float ballVX = ball->GetVelocityX();

        if (ballVX < 0.0f) {
            // Ball moving away from AI -> Idle
            m_State = AIState::Idle;
        } else {
            // Ball moving towards AI
            if (m_Difficulty == AIDifficulty::Hard) {
                m_State = AIState::Predicting;
            } else {
                m_State = AIState::Tracking;
            }
        }

        m_ReactionDelayTimer += deltaTime;

        // Only update our target position based on reaction time
        if (m_ReactionDelayTimer >= m_ReactionTime) {
            
            switch (m_State) {
                case AIState::Idle:
                    // Return to center
                    m_TargetY = (screenHeight / 2.0f);
                    break;

                case AIState::Tracking:
                    // Target the center of the ball
                    m_TargetY = ballAABB.y + (ballAABB.h / 2.0f);
                    // Add a little bit of error on easy/medium so it doesn't track perfectly
                    if (m_Difficulty == AIDifficulty::Easy) {
                        m_TargetY += (rand() % 60) - 30; 
                    } else if (m_Difficulty == AIDifficulty::Medium) {
                        m_TargetY += (rand() % 30) - 15;
                    }
                    break;

                case AIState::Predicting:
                    m_TargetY = CalculatePredictedY(ball, screenHeight);
                    break;
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

    float AIPaddle::CalculatePredictedY(const Ball* ball, int screenHeight) {
        VECTOR::AABB ballAABB = ball->GetAABB();
        VECTOR::AABB paddleAABB = GetAABB();
        
        float ballX = ballAABB.x;
        float ballY = ballAABB.y;
        float ballVX = ball->GetVelocityX();
        float ballVY = ball->GetVelocityY();

        if (ballVX <= 0.0f) return screenHeight / 2.0f;

        float timeToReach = (paddleAABB.x - (ballX + ballAABB.w)) / ballVX;
        if (timeToReach < 0.0f) timeToReach = 0.0f;

        float expectedY = ballY + (ballVY * timeToReach);
        float ballSize = ballAABB.h;

        // Simulate bounces
        int maxBounces = 10;
        while (maxBounces > 0 && (expectedY < 0.0f || expectedY + ballSize > screenHeight)) {
            if (expectedY < 0.0f) {
                expectedY = -expectedY; // Bounce off top
            } else if (expectedY + ballSize > screenHeight) {
                expectedY = 2.0f * (screenHeight - ballSize) - expectedY; // Bounce off bottom
            }
            maxBounces--;
        }

        // Return center of ball
        return expectedY + (ballSize / 2.0f);
    }

} // namespace Game
