#include "Game/Entities/Paddle.hpp"
#include <algorithm>

namespace Game {

    Paddle::Paddle(float x, float y, SDL_Scancode upKey, SDL_Scancode downKey)
        : VECTOR::GameObject(1, "Player1", x, y), m_Speed(400.0f), m_Width(20.0f), m_Height(100.0f),
          m_UpKey(upKey), m_DownKey(downKey)
    {
    }

    void Paddle::Update(float deltaTime, const VECTOR::InputManager* inputManager, int screenHeight) {
        if (inputManager->IsKeyPressed(m_UpKey)) {
            m_Position.y -= m_Speed * deltaTime;
        }
        if (inputManager->IsKeyPressed(m_DownKey)) {
            m_Position.y += m_Speed * deltaTime;
        }

        // Clamp to screen
        if (m_Position.y < 0.0f) {
            m_Position.y = 0.0f;
        }
        if (m_Position.y + m_Height > static_cast<float>(screenHeight)) {
            m_Position.y = static_cast<float>(screenHeight) - m_Height;
        }
    }

    void Paddle::Render(VECTOR::Renderer* renderer) const {
        renderer->DrawRect(
            static_cast<int>(m_Position.x),
            static_cast<int>(m_Position.y),
            static_cast<int>(m_Width),
            static_cast<int>(m_Height),
            255, 255, 255
        );
    }

    VECTOR::AABB Paddle::GetAABB() const {
        return { m_Position.x, m_Position.y, m_Width, m_Height };
    }

    void Paddle::Reset(float x, float y) {
        m_Position.x = x;
        m_Position.y = y;
    }

} // namespace Game
