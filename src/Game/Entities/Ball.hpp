#pragma once

#include "Engine/Math/GameObject.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace Game {

    class Ball : public VECTOR::GameObject {
    public:
        Ball(float x, float y);

        void Update(float deltaTime, int screenWidth, int screenHeight);
        void Render(VECTOR::Renderer* renderer) const override;

        VECTOR::AABB GetAABB() const;
        void BounceX();
        void BounceY();
        void Reset(float x, float y);
        
        bool IsOutOfBoundsLeft() const;
        bool IsOutOfBoundsRight(int screenWidth) const;

        float GetSpeed() const { return m_Speed; }
        void SetSpeed(float speed) { m_Speed = speed; }
        void SetVelocity(float x, float y) { m_Velocity.x = x; m_Velocity.y = y; }
        float GetVelocityX() const { return m_Velocity.x; }
        float GetVelocityY() const { return m_Velocity.y; }

    private:
        VECTOR::Vector2D m_Velocity;
        float m_Size;
        float m_Speed;
    };

} // namespace Game
