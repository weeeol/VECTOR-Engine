#pragma once

#include "Engine/Math/Math.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include <string>

namespace VECTOR {

    class GameObject {
    public:
        GameObject(int id, const std::string& name, float x, float y)
            : m_ID(id), m_Name(name), m_Position(x, y) {}
        virtual ~GameObject() = default;

        virtual void Update(float deltaTime) {}
        virtual void Render(Renderer* renderer) const = 0;

        Vector2D GetPosition() const { return m_Position; }
        void SetPosition(float x, float y) { m_Position.x = x; m_Position.y = y; }

        int GetID() const { return m_ID; }
        std::string GetName() const { return m_Name; }

    protected:
        int m_ID;
        std::string m_Name;
        Vector2D m_Position;
    };

} // namespace VECTOR
