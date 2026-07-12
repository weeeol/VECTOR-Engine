#pragma once

#include "Engine/GameObject.hpp"
#include "Engine/Renderer.hpp"

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

    private:
        VECTOR::Vector2D m_Velocity;
        float m_Size;
        float m_Speed;
    };

} // namespace Game
