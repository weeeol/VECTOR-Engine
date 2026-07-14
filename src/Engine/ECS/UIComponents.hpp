#pragma once
#include <string>
#include <functional>
#include <glm/glm.hpp>

namespace VECTOR {
    struct UIRectComponent {
        int x, y;
        int width, height;
        glm::vec4 color;
        bool isVisible = true;

        UIRectComponent() : x(0), y(0), width(0), height(0), color(1.0f) {}
        UIRectComponent(int x, int y, int w, int h, const glm::vec4& c) : x(x), y(y), width(w), height(h), color(c) {}
    };

    struct UITextComponent {
        std::string text;
        int fontSize;
        glm::vec4 color;
        int offsetX, offsetY; // Offset relative to UIRectComponent position if any

        UITextComponent() : text(""), fontSize(24), color(1.0f), offsetX(0), offsetY(0) {}
        UITextComponent(const std::string& t, int size, const glm::vec4& c) : text(t), fontSize(size), color(c), offsetX(0), offsetY(0) {}
    };

    struct UIButtonComponent {
        glm::vec4 normalColor;
        glm::vec4 hoverColor;
        std::function<void()> onClick;
        bool isHovered = false;
        bool wasPressed = false;

        UIButtonComponent() : normalColor(1.0f), hoverColor(1.0f) {}
        UIButtonComponent(const glm::vec4& normal, const glm::vec4& hover, std::function<void()> cb)
            : normalColor(normal), hoverColor(hover), onClick(cb) {}
    };

    struct UISliderComponent {
        float value; // 0.0f to 1.0f
        bool isDragging = false;
        std::function<void(float)> onValueChanged;

        UISliderComponent() : value(0.0f) {}
        UISliderComponent(float initialVal, std::function<void(float)> cb) : value(initialVal), onValueChanged(cb) {}
    };
}
