#include "Engine/UI/UISystem.hpp"

namespace VECTOR {

    UISystem::UISystem(InputManager* inputManager) : m_Input(inputManager) {}

    void UISystem::Update(Registry& registry, float deltaTime) {
        if (!m_Input) return;

        int mx = m_Input->GetMouseX();
        int my = m_Input->GetMouseY();
        bool isPressedNow = m_Input->IsMouseButtonPressed(MouseButton::Left);

        // Update Buttons
        registry.View<UIRectComponent, UIButtonComponent>([&](Entity entity) {
            auto& rect = registry.GetComponent<UIRectComponent>(entity);
            auto& btn = registry.GetComponent<UIButtonComponent>(entity);

            if (!rect.isVisible) return;

            btn.isHovered = (mx >= rect.x && mx <= rect.x + rect.width &&
                             my >= rect.y && my <= rect.y + rect.height);

            if (btn.isHovered && isPressedNow && !btn.wasPressed) {
                if (btn.onClick) {
                    btn.onClick();
                }
            }

            btn.wasPressed = isPressedNow;
            rect.color = btn.isHovered ? btn.hoverColor : btn.normalColor;
        });

        // Update Sliders
        registry.View<UIRectComponent, UISliderComponent>([&](Entity entity) {
            auto& rect = registry.GetComponent<UIRectComponent>(entity);
            auto& slider = registry.GetComponent<UISliderComponent>(entity);

            if (!rect.isVisible) return;

            if (isPressedNow && !slider.isDragging) {
                // Pad Y for easier clicking
                if (mx >= rect.x && mx <= rect.x + rect.width &&
                    my >= rect.y - 10 && my <= rect.y + rect.height + 10) {
                    slider.isDragging = true;
                }
            }

            if (!isPressedNow) {
                slider.isDragging = false;
            }

            if (slider.isDragging) {
                float relativeX = (float)(mx - rect.x);
                slider.value = relativeX / (float)rect.width;
                if (slider.value < 0.0f) slider.value = 0.0f;
                if (slider.value > 1.0f) slider.value = 1.0f;

                if (slider.onValueChanged) {
                    slider.onValueChanged(slider.value);
                }
            }
        });
    }

} // namespace VECTOR
