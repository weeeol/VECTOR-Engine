#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    class UIElement {
    public:
        virtual ~UIElement() = default;

        virtual void Update(InputManager* input, float deltaTime) = 0;
        virtual void Render(Renderer* renderer) = 0;

        bool IsVisible() const { return m_IsVisible; }
        void SetVisible(bool visible) { m_IsVisible = visible; }

    protected:
        int m_X = 0;
        int m_Y = 0;
        int m_Width = 0;
        int m_Height = 0;
        bool m_IsVisible = true;
    };

} // namespace VECTOR
