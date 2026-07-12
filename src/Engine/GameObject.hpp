#pragma once

#include "Engine/Math.hpp"
#include "Engine/Renderer.hpp"

namespace VECTOR {

    class GameObject {
    public:
        GameObject(float x, float y) : m_Position(x, y) {}
        virtual ~GameObject() = default;

        virtual void Update(float deltaTime) {}
        virtual void Render(Renderer* renderer) const = 0;

        Vector2D GetPosition() const { return m_Position; }
        void SetPosition(float x, float y) { m_Position.x = x; m_Position.y = y; }

    protected:
        Vector2D m_Position;
    };

} // namespace VECTOR
