#include "InputManager.hpp"

namespace VECTOR {

    InputManager::InputManager() {
        // Initialize keyboard state pointer
        m_KeyboardState = SDL_GetKeyboardState(nullptr);
    }

    InputManager::~InputManager() {
    }

    void InputManager::Update() {
        // SDL_PumpEvents() is called automatically by SDL_PollEvent() in the Application loop.
        // Therefore, m_KeyboardState is kept up-to-date automatically by SDL.
        // We just need this method in case we want to add "just pressed" or "just released" logic later.
    }

    bool InputManager::IsKeyPressed(SDL_Scancode key) const {
        if (m_KeyboardState) {
            return m_KeyboardState[key] == 1;
        }
        return false;
    }

} // namespace VECTOR
