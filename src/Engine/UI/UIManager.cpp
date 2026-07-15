#include "Engine/UI/UIManager.hpp"

namespace VECTOR {

    void UIManager::AddElement(std::shared_ptr<UIElement> element) {
        m_Elements.push_back(element);
    }

    void UIManager::Clear() {
        m_Elements.clear();
    }

    void UIManager::Update(InputManager* input, float deltaTime) {
        for (size_t i = 0; i < m_Elements.size(); ++i) {
            auto element = m_Elements[i]; // Keep a strong reference
            element->Update(input, deltaTime);
        }
    }

    void UIManager::Render(Renderer* renderer) {
        for (auto& element : m_Elements) {
            element->Render(renderer);
        }
    }

} // namespace VECTOR
