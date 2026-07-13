#pragma once

#include "Engine/UI/UIElement.hpp"
#include <string>
#include <functional>

namespace VECTOR {

    class UISlider : public UIElement {
    public:
        // value should be between 0.0f and 1.0f
        UISlider(int x, int y, int width, int height, float initialValue, std::function<void(float)> onValueChanged);
        ~UISlider() override = default;

        void Update(InputManager* input, float deltaTime) override;
        void Render(Renderer* renderer) override;

        float GetValue() const { return m_Value; }
        void SetValue(float value) {
            m_Value = value;
            if (m_Value < 0.0f) m_Value = 0.0f;
            if (m_Value > 1.0f) m_Value = 1.0f;
        }

    private:
        float m_Value;
        bool m_IsDragging;
        std::function<void(float)> m_OnValueChanged;
    };

} // namespace VECTOR
