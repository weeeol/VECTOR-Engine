#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    InputManager::InputManager() : m_MouseState(0), m_PrevMouseState(0), m_MouseX(0), m_MouseY(0) {
        // Initialize keyboard state pointer
        m_KeyboardState = SDL_GetKeyboardState(nullptr);
    }

    InputManager::~InputManager() {
    }

    void InputManager::Update() {
        m_PrevMouseState = m_MouseState;
        m_MouseState = SDL_GetMouseState(&m_MouseX, &m_MouseY);
    }

    bool InputManager::IsKeyPressed(SDL_Scancode key) const {
        if (m_KeyboardState) {
            return m_KeyboardState[key] == 1;
        }
        return false;
    }

    bool InputManager::IsMouseButtonPressed(int button) const {
        return (m_MouseState & SDL_BUTTON(button)) != 0;
    }
    
    bool InputManager::IsMouseButtonJustPressed(int button) const {
        bool isPressedNow = (m_MouseState & SDL_BUTTON(button)) != 0;
        bool wasPressedBefore = (m_PrevMouseState & SDL_BUTTON(button)) != 0;
        return isPressedNow && !wasPressedBefore;
    }

} // namespace VECTOR
