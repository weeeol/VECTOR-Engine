#pragma once

#include "Engine/UI/UIElement.hpp"
#include <vector>
#include <memory>

namespace VECTOR {

    class UIManager {
    public:
        UIManager() = default;
        ~UIManager() = default;

        void AddElement(std::shared_ptr<UIElement> element);
        void Clear();

        void Update(InputManager* input, float deltaTime);
        void Render(Renderer* renderer);

    private:
        std::vector<std::shared_ptr<UIElement>> m_Elements;
    };

} // namespace VECTOR
