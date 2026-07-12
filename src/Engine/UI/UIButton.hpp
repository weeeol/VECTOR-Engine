#pragma once

#include "Engine/UI/UIElement.hpp"
#include <string>
#include <functional>
#include <SDL.h>

namespace VECTOR {

    class UIButton : public UIElement {
    public:
        UIButton(int x, int y, int width, int height, const std::string& text, std::function<void()> onClick);

        void Update(InputManager* input, float deltaTime) override;
        void Render(Renderer* renderer) override;

        void SetText(const std::string& text) { m_Text = text; }
        
        // Styling options
        void SetColors(SDL_Color normal, SDL_Color hover, SDL_Color textCol) {
            m_NormalColor = normal;
            m_HoverColor = hover;
            m_TextColor = textCol;
        }

    private:
        std::string m_Text;
        std::function<void()> m_OnClick;
        bool m_IsHovered;

        SDL_Color m_NormalColor = { 50, 50, 50, 255 };
        SDL_Color m_HoverColor = { 100, 100, 100, 255 };
        SDL_Color m_TextColor = { 255, 255, 255, 255 };
    };

} // namespace VECTOR
