#include "Engine/UI/UISlider.hpp"

namespace VECTOR {

    UISlider::UISlider(int x, int y, int width, int height, float initialValue, std::function<void(float)> onValueChanged)
        : m_Value(initialValue), m_IsDragging(false), m_OnValueChanged(onValueChanged) 
    {
        m_X = x;
        m_Y = y;
        m_Width = width;
        m_Height = height;

        if (m_Value < 0.0f) m_Value = 0.0f;
        if (m_Value > 1.0f) m_Value = 1.0f;
    }

    void UISlider::Update(InputManager* input, float deltaTime) {
        if (!m_IsVisible) return;

        int mouseX = input->GetMouseX();
        int mouseY = input->GetMouseY();
        bool isMouseDown = input->IsMouseButtonPressed(SDL_BUTTON_LEFT);

        // Check if dragging started
        if (isMouseDown && !m_IsDragging) {
            if (mouseX >= m_X && mouseX <= m_X + m_Width &&
                mouseY >= m_Y - 10 && mouseY <= m_Y + m_Height + 10) { // pad Y a bit for easier clicking
                m_IsDragging = true;
            }
        }

        // Check if dragging stopped
        if (!isMouseDown) {
            m_IsDragging = false;
        }

        // Update value if dragging
        if (m_IsDragging) {
            float relativeX = (float)(mouseX - m_X);
            m_Value = relativeX / (float)m_Width;
            if (m_Value < 0.0f) m_Value = 0.0f;
            if (m_Value > 1.0f) m_Value = 1.0f;

            if (m_OnValueChanged) {
                m_OnValueChanged(m_Value);
            }
        }
    }

    void UISlider::Render(Renderer* renderer) {
        if (!m_IsVisible) return;

        // Draw track background (gray)
        renderer->DrawRect(m_X, m_Y, m_Width, m_Height, 100, 100, 100, 255);

        // Draw filled track (green)
        int fillWidth = (int)(m_Width * m_Value);
        renderer->DrawRect(m_X, m_Y, fillWidth, m_Height, 50, 200, 50, 255);

        // Draw knob (white)
        int knobWidth = 10;
        int knobHeight = m_Height + 10;
        int knobX = m_X + fillWidth - (knobWidth / 2);
        int knobY = m_Y - 5;
        renderer->DrawRect(knobX, knobY, knobWidth, knobHeight, 255, 255, 255, 255);
    }

} // namespace VECTOR
