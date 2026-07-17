#include "Engine/Input/InputManager.hpp"

namespace VECTOR {

    InputManager::InputManager() : m_MouseState(0), m_MouseX(0), m_MouseY(0) {
        // Initialize keyboard state pointer
        m_KeyboardState = SDL_GetKeyboardState(nullptr);
        for(int i=0; i<6; ++i) m_MouseJustPressed[i] = false;
        for(int i=0; i<SDL_SCANCODE_COUNT; ++i) m_KeyJustPressed[i] = false;
    }

    InputManager::~InputManager() {
    }

    void InputManager::PrepareForEvents() {
        // Handled per frame if needed, but we clear it in ClearJustPressed now
    }

    void InputManager::ProcessEvent(const SDL_Event& e) {
        if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (e.button.button < 6) {
                m_MouseJustPressed[e.button.button] = true;
            }
        } else if (e.type == SDL_EVENT_KEY_DOWN) {
            if (!e.key.repeat && e.key.scancode < SDL_SCANCODE_COUNT) {
                m_KeyJustPressed[e.key.scancode] = true;
            }
        }
    }

    void InputManager::Update() {
        m_MouseState = SDL_GetMouseState(&m_MouseX, &m_MouseY);
    }

    void InputManager::ClearJustPressed() {
        for(int i=0; i<6; ++i) m_MouseJustPressed[i] = false;
        for(int i=0; i<SDL_SCANCODE_COUNT; ++i) m_KeyJustPressed[i] = false;
    }

    bool InputManager::IsKeyPressed(SDL_Scancode key) const {
        if (m_KeyboardState) {
            return m_KeyboardState[key] == 1;
        }
        return false;
    }

    bool InputManager::IsKeyJustPressed(SDL_Scancode key) const {
        if (key < SDL_SCANCODE_COUNT) {
            return m_KeyJustPressed[key];
        }
        return false;
    }

    bool InputManager::IsMouseButtonPressed(int button) const {
        return (m_MouseState & SDL_BUTTON_MASK(button)) != 0;
    }
    
    bool InputManager::IsMouseButtonJustPressed(int button) const {
        if (button < 6) {
            return m_MouseJustPressed[button];
        }
        return false;
    }

} // namespace VECTOR
