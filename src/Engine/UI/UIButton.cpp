#include "Engine/UI/UIButton.hpp"

namespace VECTOR {

    UIButton::UIButton(int x, int y, int width, int height, const std::string& text, std::function<void()> onClick)
        : m_Text(text), m_OnClick(onClick), m_IsHovered(false)
    {
        m_X = x;
        m_Y = y;
        m_Width = width;
        m_Height = height;
    }

    void UIButton::Update(InputManager* input, float deltaTime) {
        if (!m_IsVisible) return;

        int mx = input->GetMouseX();
        int my = input->GetMouseY();

        // Check AABB collision with mouse
        m_IsHovered = (mx >= m_X && mx <= m_X + m_Width && my >= m_Y && my <= m_Y + m_Height);

        if (m_IsHovered && input->IsMouseButtonJustPressed(SDL_BUTTON_LEFT)) {
            if (m_OnClick) {
                m_OnClick();
            }
        }
    }

    void UIButton::Render(Renderer* renderer) {
        if (!m_IsVisible) return;

        SDL_Color bgColor = m_IsHovered ? m_HoverColor : m_NormalColor;

        // Draw border for a polished look
        if (m_IsHovered) {
            renderer->DrawRect(m_X - 3, m_Y - 3, m_Width + 6, m_Height + 6, 255, 255, 255, 200); // Glowing white border
        } else {
            renderer->DrawRect(m_X - 1, m_Y - 1, m_Width + 2, m_Height + 2, 80, 80, 80, 255); // Subtle grey border
        }

        // Draw button background
        renderer->DrawRect(m_X, m_Y, m_Width, m_Height, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        
        // Draw button text centered approximately
        int textX = m_X + (m_Width / 2) - (m_Text.length() * 6);
        int textY = m_Y + (m_Height / 2) - 12;
        renderer->DrawText(m_Text, textX, textY, m_TextColor.r, m_TextColor.g, m_TextColor.b, 24);
    }

} // namespace VECTOR
