#include "Game/Entities/Ball.hpp"

namespace Game {

    Ball::Ball(float x, float y)
        : VECTOR::GameObject(3, "GameBall", x, y), m_Size(15.0f), m_Speed(400.0f)
    {
        // 0.707106f is 1 / sqrt(2), which normalizes the (1, 1) vector
        m_Velocity = VECTOR::Vector2D(0.707106f, 0.707106f) * m_Speed;
    }

    void Ball::Update(float deltaTime, int screenWidth, int screenHeight) {
        m_Position += m_Velocity * deltaTime;

        // Bounce off top and bottom walls
        if (m_Position.y < 0.0f) {
            m_Position.y = 0.0f;
            BounceY();
        } else if (m_Position.y + m_Size > static_cast<float>(screenHeight)) {
            m_Position.y = static_cast<float>(screenHeight) - m_Size;
            BounceY();
        }
    }

    void Ball::Render(VECTOR::Renderer* renderer) const {
        renderer->DrawRect(
            static_cast<int>(m_Position.x),
            static_cast<int>(m_Position.y),
            static_cast<int>(m_Size),
            static_cast<int>(m_Size),
            255, 255, 255
        );
    }

    VECTOR::AABB Ball::GetAABB() const {
        return { m_Position.x, m_Position.y, m_Size, m_Size };
    }

    void Ball::BounceX() {
        m_Velocity.x = -m_Velocity.x;
    }

    void Ball::BounceY() {
        m_Velocity.y = -m_Velocity.y;
    }

    void Ball::Reset(float x, float y) {
        m_Position = VECTOR::Vector2D(x, y);
        m_Speed = 400.0f; // Reset to base speed
        
        // Randomize initial direction slightly, maintaining normalized vector length
        float dirX = m_Velocity.x > 0 ? -0.707106f : 0.707106f;
        float dirY = m_Velocity.y > 0 ? -0.707106f : 0.707106f;
        m_Velocity.x = dirX * m_Speed;
        m_Velocity.y = dirY * m_Speed;
    }

    bool Ball::IsOutOfBoundsLeft() const {
        return m_Position.x < 0.0f;
    }

    bool Ball::IsOutOfBoundsRight(int screenWidth) const {
        return m_Position.x + m_Size > static_cast<float>(screenWidth);
    }

} // namespace Game
